(define private-value 99)
(define incoming (ipc:receive 1))
(ipc:send 2 (+ incoming 1))
(cond ((= incoming 42) (= private-value 99)) (t nil))
