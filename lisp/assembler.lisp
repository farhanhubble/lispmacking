; SPDX-License-Identifier: MPL-2.0
;
; RV64IM instruction encoding used by the Milestone 5 compiler. Each top-level
; form occupies one line so the same file can be streamed into the serial REPL.

(define rv-u12 (lambda (n) (cond ((< n 0) (+ n 4096)) (t n))))
(define rv-u13 (lambda (n) (cond ((< n 0) (+ n 8192)) (t n))))
(define rv-u21 (lambda (n) (cond ((< n 0) (+ n 2097152)) (t n))))
(define rv-r (lambda (opcode rd funct3 rs1 rs2 funct7) (+ (+ (+ (+ (+ opcode (* rd 128)) (* funct3 4096)) (* rs1 32768)) (* rs2 1048576)) (* funct7 33554432))))
(define rv-i-raw (lambda (opcode rd funct3 rs1 immediate) (+ (+ (+ (+ opcode (* rd 128)) (* funct3 4096)) (* rs1 32768)) (* (rv-u12 immediate) 1048576))))
(define rv-i (lambda (opcode rd funct3 rs1 immediate) (cond ((< immediate -2048) (error 'assembler:i-immediate-out-of-range)) ((> immediate 2047) (error 'assembler:i-immediate-out-of-range)) (t (rv-i-raw opcode rd funct3 rs1 immediate)))))
(define rv-s-raw (lambda (opcode funct3 rs1 rs2 immediate) ((lambda (encoded) (+ (+ (+ (+ opcode (* (mod encoded 32) 128)) (* funct3 4096)) (* rs1 32768)) (+ (* rs2 1048576) (* (quotient encoded 32) 33554432)))) (rv-u12 immediate))))
(define rv-s (lambda (opcode funct3 rs1 rs2 immediate) (cond ((< immediate -2048) (error 'assembler:s-immediate-out-of-range)) ((> immediate 2047) (error 'assembler:s-immediate-out-of-range)) (t (rv-s-raw opcode funct3 rs1 rs2 immediate)))))
(define rv-b-raw (lambda (opcode funct3 rs1 rs2 offset) ((lambda (encoded) (+ (+ (+ (+ (+ (+ opcode (* (mod (quotient encoded 2048) 2) 128)) (* (mod (quotient encoded 2) 16) 256)) (* funct3 4096)) (* rs1 32768)) (* rs2 1048576)) (+ (* (mod (quotient encoded 32) 64) 33554432) (* (mod (quotient encoded 4096) 2) 2147483648)))) (rv-u13 offset))))
(define rv-b (lambda (opcode funct3 rs1 rs2 offset) (cond ((< offset -4096) (error 'assembler:b-immediate-out-of-range)) ((> offset 4094) (error 'assembler:b-immediate-out-of-range)) ((= (mod offset 2) 1) (error 'assembler:b-immediate-misaligned)) (t (rv-b-raw opcode funct3 rs1 rs2 offset)))))
(define rv-j-raw (lambda (opcode rd offset) ((lambda (encoded) (+ (+ (+ (+ (+ opcode (* rd 128)) (* (mod (quotient encoded 4096) 256) 4096)) (* (mod (quotient encoded 2048) 2) 1048576)) (* (mod (quotient encoded 2) 1024) 2097152)) (* (mod (quotient encoded 1048576) 2) 2147483648))) (rv-u21 offset))))
(define rv-j (lambda (opcode rd offset) (cond ((< offset -1048576) (error 'assembler:j-immediate-out-of-range)) ((> offset 1048574) (error 'assembler:j-immediate-out-of-range)) ((= (mod offset 2) 1) (error 'assembler:j-immediate-misaligned)) (t (rv-j-raw opcode rd offset)))))
(define rv-addi (lambda (rd rs1 immediate) (rv-i 19 rd 0 rs1 immediate)))
(define rv-add (lambda (rd rs1 rs2) (rv-r 51 rd 0 rs1 rs2 0)))
(define rv-sub (lambda (rd rs1 rs2) (rv-r 51 rd 0 rs1 rs2 32)))
(define rv-mul (lambda (rd rs1 rs2) (rv-r 51 rd 0 rs1 rs2 1)))
(define rv-slt (lambda (rd rs1 rs2) (rv-r 51 rd 2 rs1 rs2 0)))
(define rv-ld (lambda (rd rs1 immediate) (rv-i 3 rd 3 rs1 immediate)))
(define rv-sd (lambda (rs2 rs1 immediate) (rv-s 35 3 rs1 rs2 immediate)))
(define rv-beq (lambda (rs1 rs2 offset) (rv-b 99 0 rs1 rs2 offset)))
(define rv-jal (lambda (rd offset) (rv-j 111 rd offset)))
(define rv-jalr (lambda (rd rs1 immediate) (rv-i 103 rd 0 rs1 immediate)))
(define rv-emit (lambda (base offset word) (bootstrap:store-u32 (+ base offset) word)))
