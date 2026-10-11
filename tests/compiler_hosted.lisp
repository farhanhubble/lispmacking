; SPDX-License-Identifier: MPL-2.0

(rv-addi 10 10 1)
(rv-add 10 5 10)
(rv-sd 10 5 0)
(rv-beq 10 0 8)
(rv-jal 1 -4)
(compiler-find-function 'second (compiler-build-map '((first (x) x) (second (x) (+ x 1))) 4096 0))
(rv-addi 10 10 2048)
(rv-beq 10 0 3)
