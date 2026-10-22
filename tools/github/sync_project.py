#!/usr/bin/env python3
"""Reconcile Lispmacking's private Project with its repository roadmap.

Default is read-only. --apply creates missing M8-M19 tracking issues, adds
repository issues to the existing private Project and reconciles closed issues.
Existing open-item status, priority, area and project visibility are preserved.
Tokens are read from private files, never persisted in the repository or printed.
"""
import argparse
import json
from pathlib import Path
import re
import time
import urllib.request

REPO = 'farhanhubble/lispmacking'
OWNER = 'farhanhubble'
PROJECT = 2
ROOT = Path(__file__).resolve().parents[2]
CONFIG = Path.home() / '.config/lispmacking'


def request(url, key, payload=None, method=None):
    req = urllib.request.Request(
        url, data=None if payload is None else json.dumps(payload).encode(),
        headers={'Authorization': 'Bearer ' + key, 'Content-Type': 'application/json',
                 'Accept': 'application/vnd.github+json'}, method=method)
    with urllib.request.urlopen(req, timeout=30) as response:
        return json.load(response)


def main():
    parser = argparse.ArgumentParser(description=__doc__)
    parser.add_argument('--apply', action='store_true')
    args = parser.parse_args()
    repo_key = (CONFIG / 'github-token').read_text().strip()
    project_key = (CONFIG / 'github-project-token').read_text().strip()

    def rest(path, payload=None, method=None):
        return request('https://api.github.com/repos/' + REPO + path,
                       repo_key, payload, method)

    def pages(path):
        items = []
        for page in range(1, 1000):
            batch = rest(path + ('&' if '?' in path else '?') +
                         f'per_page=100&page={page}')
            items.extend(batch)
            if len(batch) < 100:
                return items
        raise RuntimeError('Unexpected pagination limit')

    def gql(query, variables=None):
        result = request('https://api.github.com/graphql', project_key,
                         {'query': query, 'variables': variables or {}})
        if result.get('errors'):
            raise RuntimeError(json.dumps(result['errors']))
        return result['data']

    project = gql('''query($owner:String!,$number:Int!) {
      user(login:$owner) { projectV2(number:$number) {
        id title url public readme fields(first:100) { nodes {
          ... on ProjectV2SingleSelectField { id name options { id name } }
        } }
      } }
    }''', {'owner': OWNER, 'number': PROJECT})['user']['projectV2']
    if project['title'] != 'Lispmacking' or project['public']:
        raise RuntimeError('Expected the existing private Lispmacking project')
    status = next(f for f in project['fields']['nodes'] if f.get('name') == 'Status')
    options = {o['name']: o['id'] for o in status['options']}
    assert 'Done' in options and 'Backlog' in options

    def project_items():
        items, cursor = [], None
        while True:
            connection = gql('''query($id:ID!,$cursor:String) {
              node(id:$id) { ... on ProjectV2 { items(first:100,after:$cursor) {
                pageInfo { hasNextPage endCursor }
                nodes { id content { ... on Issue { id number repository { nameWithOwner } } }
                  fieldValues(first:30) { nodes {
                    ... on ProjectV2ItemFieldSingleSelectValue {
                      name field { ... on ProjectV2SingleSelectField { name } }
                    }
                  } }
                }
              } } }
            }''', {'id': project['id'], 'cursor': cursor})['node']['items']
            items.extend(connection['nodes'])
            if not connection['pageInfo']['hasNextPage']:
                return items
            cursor = connection['pageInfo']['endCursor']

    existing = project_items()
    milestones = pages('/milestones?state=all')
    issues = [i for i in pages('/issues?state=all') if 'pull_request' not in i]
    text = (ROOT / 'docs/future-milestones.md').read_text()
    parts = re.split(r'^## (M\d+ — .+)\n', text, flags=re.M)
    definitions = list(zip(parts[1::2], parts[2::2]))
    assert len(definitions) == 12
    trackers = []
    for title, body in definitions:
        milestone = next(m for m in milestones if m['title'] == title)
        matches = [i for i in issues if i['title'] == title]
        if len(matches) > 1:
            raise RuntimeError('Duplicate tracking issues for ' + title)
        if matches:
            issue = matches[0]
            if not issue.get('milestone') or issue['milestone']['number'] != milestone['number']:
                raise RuntimeError('Wrong milestone assignment for ' + title)
        else:
            print('Create tracking issue: ' + title, flush=True)
            if not args.apply:
                continue
            content = ('Milestone tracking issue. Implementation has not started.\n\n' +
                       body.strip() + '\n\n## Tracking\n\n' +
                       '[Private project](' + project['url'] + ') · ' +
                       '[Repository milestone](' + milestone['html_url'] + ') · ' +
                       '[Detailed roadmap](https://github.com/' + REPO +
                       '/blob/main/docs/future-milestones.md)\n\n' +
                       'Close only when every acceptance criterion has evidence on its required targets. '
                       'Break implementation into linked deliverable issues as work begins.\n')
            issue = rest('/issues', {'title': title, 'body': content,
                                    'milestone': milestone['number']}, 'POST')
            issues.append(issue)
        trackers.append(issue)

    by_content = {i['content']['id']: i for i in existing
                  if i.get('content') and i['content'].get('id')}
    for issue in issues:
        item = by_content.get(issue['node_id'])
        if item is None:
            print(f"Add #{issue['number']} to Project", flush=True)
            if not args.apply:
                continue
            item = gql('''mutation($project:ID!,$content:ID!) {
              addProjectV2ItemById(input:{projectId:$project,contentId:$content}) {
                item { id }
              }
            }''', {'project': project['id'], 'content': issue['node_id']})['addProjectV2ItemById']['item']
            item['fieldValues'] = {'nodes': []}
        current = next((v['name'] for v in item['fieldValues']['nodes']
                        if v.get('field', {}).get('name') == 'Status'), None)
        desired = 'Done' if issue['state'] == 'closed' else (current or 'Backlog')
        if current != desired:
            print(f"Set #{issue['number']}: {desired}", flush=True)
            if args.apply:
                gql('''mutation($project:ID!,$item:ID!,$field:ID!,$value:String!) {
                  updateProjectV2ItemFieldValue(input:{projectId:$project,itemId:$item,
                    fieldId:$field,value:{singleSelectOptionId:$value}}) { projectV2Item { id } }
                }''', {'project': project['id'], 'item': item['id'],
                      'field': status['id'], 'value': options[desired]})
    if args.apply:
        # Project reads can briefly lag successful item/status mutations.
        for attempt in range(5):
            final = project_items()
            final_by_id = {i['content']['id']: i for i in final
                           if i.get('content') and i['content'].get('id')}
            pending = []
            for issue in issues:
                item = final_by_id.get(issue['node_id'])
                if item is None:
                    pending.append(issue['number'])
                elif issue['state'] == 'closed' and not any(
                        v.get('name') == 'Done' and
                        v.get('field', {}).get('name') == 'Status'
                        for v in item['fieldValues']['nodes']):
                    pending.append(issue['number'])
            if not pending:
                break
            if attempt < 4:
                time.sleep(2)
        if pending:
            raise RuntimeError(f'Project verification still pending for issues {pending}; rerun the audit')
        print(f'Verified {len(issues)} repository issues in {project["url"]}', flush=True)
        for issue in trackers:
            print(issue['title'] + ': ' + issue['html_url'], flush=True)
    else:
        print('Read-only audit complete. Use --apply to reconcile.', flush=True)


if __name__ == '__main__':
    main()
