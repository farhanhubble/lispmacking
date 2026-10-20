(define private-value 41)
(define twice (lambda (x) (+ x x)))
(ipc:send 1 (twice 21))
(cond ((= (ipc:receive 2) 43) (= private-value 41)) (t nil))
