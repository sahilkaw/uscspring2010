# Blocking Matrix Multiply program
		.data
amat:
		.word   1,  2,  3,  4,  5,  6,  7,  8,  9, 10, 11, 12
		.word  13, 14, 15, 16, 17, 18, 19, 20, 21, 22, 23, 24
		.word  25, 26, 27, 28, 29, 30, 31, 32, 33, 34, 35, 36
		.word  37, 38, 39, 40, 41, 42, 43, 44, 45, 46, 47, 48
		.word  49, 50, 51, 52, 53, 54, 55, 56, 57, 58, 59, 60
		.word  61, 62, 63, 64, 65, 66, 67, 68, 69, 70, 71, 72
		.word  73, 74, 75, 76, 77, 78, 79, 80, 81, 82, 83, 84
		.word  85, 86, 87, 88, 89, 90, 91, 92, 93, 94, 95, 96
		.word  97, 98, 99,100,101,102,103,104,105,106,107,108
		.word 109,110,111,112,113,114,115,116,117,118,119,120
		.word 121,122,123,124,125,126,127,128,129,130,131,132
		.word 133,134,135,136,137,138,139,140,141,142,143,144

bmat:
		.word 133,134,135,136,137,138,139,140,141,142,143,144
		.word 121,122,123,124,125,126,127,128,129,130,131,132
		.word 109,110,111,112,113,114,115,116,117,118,119,120
		.word  97, 98, 99,100,101,102,103,104,105,106,107,108
		.word  85, 86, 87, 88, 89, 90, 91, 92, 93, 94, 95, 96
		.word  73, 74, 75, 76, 77, 78, 79, 80, 81, 82, 83, 84
		.word  61, 62, 63, 64, 65, 66, 67, 68, 69, 70, 71, 72
		.word  49, 50, 51, 52, 53, 54, 55, 56, 57, 58, 59, 60
		.word  37, 38, 39, 40, 41, 42, 43, 44, 45, 46, 47, 48
		.word  25, 26, 27, 28, 29, 30, 31, 32, 33, 34, 35, 36
		.word  13, 14, 15, 16, 17, 18, 19, 20, 21, 22, 23, 24
		.word   1,  2,  3,  4,  5,  6,  7,  8,  9, 10, 11, 12

cmat:
		.word   0,  0,  0,  0,  0,  0,  0,  0,  0,  0,  0,  0
		.word   0,  0,  0,  0,  0,  0,  0,  0,  0,  0,  0,  0
		.word   0,  0,  0,  0,  0,  0,  0,  0,  0,  0,  0,  0
		.word   0,  0,  0,  0,  0,  0,  0,  0,  0,  0,  0,  0
		.word   0,  0,  0,  0,  0,  0,  0,  0,  0,  0,  0,  0
		.word   0,  0,  0,  0,  0,  0,  0,  0,  0,  0,  0,  0
		.word   0,  0,  0,  0,  0,  0,  0,  0,  0,  0,  0,  0
		.word   0,  0,  0,  0,  0,  0,  0,  0,  0,  0,  0,  0
		.word   0,  0,  0,  0,  0,  0,  0,  0,  0,  0,  0,  0
		.word   0,  0,  0,  0,  0,  0,  0,  0,  0,  0,  0,  0
		.word   0,  0,  0,  0,  0,  0,  0,  0,  0,  0,  0,  0
		.word   0,  0,  0,  0,  0,  0,  0,  0,  0,  0,  0,  0

bs:		.word 3
n:		.word 12

nline:  .asciiz "\n"				#Define new line string
sp:		.asciiz " "
msga: .asciiz "Matrix A is: \n"
msgb: .asciiz "Matrix B is: \n"
msgc: .asciiz "Matrix C=A*B is: \n"

		.text
		.globl main
main:

		la	$s0, bs	
		lw	$s0, 0($s0)
		la	$s1, n
		lw	$s1, 0($s1)
		la	$s2, amat
		la	$s3, bmat
		la	$s4, cmat

		la	$a0, msga
		la 	$a1, amat
		jal	PMAT 
		la	$a0, msgb
		la 	$a1, bmat
		jal	PMAT 

# YOUR CODE HERE
# Vars:
# $s0 = block size
# $s1 = N
# $t0 = i
# $t1 = j
# $t2 = k
# $t3 = ii
# $t4 = jj
# $t5 = kk
# $t6 = value of A
# $t7 = value of B/value of A*B
# $t8 = value of C+A*B
# $t9 = address of word being altered or acquired/conditional

		sub $t0, $0, $s0
LOOPA:	add $t0, $t0, $s0
		bge $t0, $s1, END

		sub $t1, $0, $s0
LOOPB:  add $t1, $t1, $s0
		bge $t1, $s1, LOOPA

		sub $t2, $0, $s0
LOOPC:  add $t2, $t2, $s0
		bge $t2, $s1, LOOPB

		addi $t3, $t0,-1
LOOPD:  addi $t3, $t3, 1
		add $t9, $t0, $s0
		bge $t3, $t9, LOOPC

		addi $t4, $t1,-1
LOOPE:  addi $t4, $t4, 1
		add $t9, $t1, $s0
		bge $t4, $t9, LOOPD

		addi $t5, $t2,-1
LOOPF:  addi $t5, $t5, 1
		add $t9, $t2, $s0
		bge $t5, $t9, LOOPE

		mulo $t9, $t3, $s1
		add $t9, $t9, $t5
		sll $t9, $t9, 2
		lw $t6, amat($t9)		# A[ii][kk]
		mulo $t9, $t5, $s1
		add $t9, $t9, $t4
		sll $t9, $t9, 2
		lw $t7, bmat($t9)		# B[kk][jj]
		mulo $t7, $t7, $t6		# A[ii][kk] * B[kk][jj]
		mulo $t9, $t3, $s1
		add $t9, $t9, $t4
		sll $t9, $t9, 2
		lw $t8, cmat($t9)
		add $t8,$t8,$t7			# C[ii][jj] =  C[ii][jj]+A[ii][kk] * B[kk][jj]
		sw $t8, cmat($t9)
		b LOOPF
		

# End CODE

END:	la	$a0, msgc
		la 	$a1, cmat
		jal	PMAT 

#   Exit
		li	 $v0,10
    	syscall


PMAT:	li	$v0,4
		syscall
		addi $a2,$0,0	
PL4:	bge	$a2,$s1,PL1
		addi $a3,$0,0
PL3:	bge	$a3,$s1,PL2

		lw	$a0,0($a1)
		li	$v0,1
		syscall
		la	$a0,sp
		li	$v0,4
		syscall
		addi $a1,$a1,4
		addi $a3,$a3,1
		b 	PL3

PL2:	addi	$a2,$a2,1
		la	$a0,nline
		li	$v0,4
		syscall
		b	PL4
PL1:	jr	$ra
