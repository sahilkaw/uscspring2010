# ee352_lab1.s
# Name:  Niels-David Yogi
# Description: Takes user input from 1-20 numbers utilizing spaces.  When inputting numbers, do not place a space
# 				after the last number. This program will then print out the inputted list and sort the list.
#				It will then print out the sorted list and calculate a truncated mean, the median (if even 
#				lower number) and mode (if frequency is same, higher number is chosen) and print these three 
#				out accordingly.
		.data
buf1:   .space  41     # receive original input in this buffer
buf2:   .space  41     # can use this buffer for any other purpose (strip spaces, etc.)


# the following are constant strings that you can use for your prompts and messages
msgin:  .asciiz "Enter up to 20 digits with one space following each digit: "
msg1:   .asciiz "Original list: "
msg2:   .asciiz "Sorted numbers: "
msg3:   .asciiz "Truncated Mean: "
msg4:   .asciiz "Median: "
msg5:   .asciiz "Mode: "

# print this string for a newline character
nline:  .asciiz "\n"

		.text
		li $t1, 0		# Length of Numbers
		la $s0, buf1	# address to buf1
		la $s1, buf2	# address to buf2
		li $s2, 0		# Mean
		li $s3, 0		# Median
		li $s4, 0		# Mode

#### Store Input ####
input:	la $a0, msgin
		li $v0, 4
		syscall 				# print the prompt string
		la $a0, ($s0)			# the address of buff1	
		la $a1, 41				# the number of words
		li $v0, 8				# specify Read Integer service
		syscall

#### Start Determine Max Numbers ####
		li $t2, 1			# start position of count
maxL:	add $s0,$s0,$t2		# move address accordingly to the counter
		lb $t3,($s0)		# store value at the specified address
		add $t2,$t2, 1		# adds the amount
		la $s0,buf1			# restores buf1 starting address
		bne $t3,10,maxL		# Branch if the byte does represent a \n char
		div	$t1,$t2,2		# Divide counter by 2 in order to get the length of actual numbers instead of the chars

#### Print Inputterd List ####
		la $a0, msg1
		li $v0, 4
		syscall 			# print the inputted list
		jal print           # call print routine. 

#### Bubble Sort ####
		ble $t1, 1, MMM			# Calculate Mean, Mode and Median if only one number

		li $t2,0 				# i counter
outer:	li $t3,0				# j counter
inner:	mulo $t7, $t3, 2
		lb $t4, buf1($t7)		# Temp = Buf1[j]
		add $t7, $t7, 2
		lb $t5, buf1($t7)		# Temp2 = Buf[j+1]
		bge $t5,$t4, check		# Branch if Temp2 >= Temp
		
		mulo $t7, $t3, 2
		sb $t5 buf1($t7)		# Buf1[j] = Buf1[j+1]
		add $t7, $t7, 2
		sb $t4, buf1($t7)		# Buf1[j+1] = Temp
check:	addi $t3,$t3,1			# increment j
		sub $t6,$t1,$t2			# Store value Length-i
		sub $t6,$t6,1
		blt $t3, $t6, inner		# if j<Length-i branch to inner loop
		addi $t2,$t2,1			# increment i
		blt $t2,$t1,outer		# if i<max branch outer
			
#### Find Mean, Median and Mode ####
MMM:	# Truncated Mean Calculation
		li $t2, 0			# start position of count
mean:	mulo $t4,$t2,2
		add $s0,$s0,$t4		# move address accordingly to the counter
		lb $t3,($s0)		# store value at the specified address
		add $t2,$t2, 1		# adds the amount
		la $s0,buf1			# restores buf1 starting address
		add $s2,$s2,$t3
		ble $t2,$t1,mean
		div $s2,$s2,$t1
		
		# Median Calculation
median: add $t3, $t1, 1
		div $t2, $t1, 2
		div $t3, $t3, 2
		beq $t2,$t3,mEven
		mulo $t2,$t2,2
		lb $s3, buf1($t2)
		b mode
mEven:	sub $t2,$t2,1			# Lower number; remove this to get the higher one
		mulo $t2,$t2,2
		lb $s3, buf1($t2)
		
		# Mode Calculation
mode:	li $s4, 0				# mode
		li $t2, 0				# number times appears
		li $t3, 0				# num2
		li $t4, 0				# num2's times it appears
		li $t5, 0				# compare number
		li $t6, 0				# counter
		mulo $t7,$t6,2
		lb $s4, buf1($t6)
mTop:	add $t2, $t2, 1
		add $t6, $t6, 1
		bgt $t6, $t1, print
		mulo $t7,$t6, 2
		lb $t5, buf1($t7)
		beq $s4, $t5, mTop
		
mBtmU:	lb $t3, buf1($t7)
		li $t4, 0				#  num2's times it appears
mBottom:add $t4, $t4, 1
		add $t6, $t6, 1
		bgt $t6, $t1, pSAL
		mulo $t7,$t6, 2
		lb $t5, buf1($t7)
		beq $t3, $t5, mBottom
		blt $t4, $t2, mBtmU
		move $s4, $t3
		move $t2, $t4
		b mBtmU

#### Print Sorted Array List ####
pSAL:	la $a0, msg2
		li $v0, 4
		syscall 			# print the inputted list
		jal print	           # call print routine. 

#### Print Mean,Median & Mode ####
pMean:	la $a0, msg3
		li $v0, 4
		syscall 				# print the prompt string
		add $a0,$s2,0	
		li $v0, 11
		syscall
		la $a0, nline
		li $v0, 4
		syscall 				# print \n

pMedian:la $a0, msg4
		li $v0, 4
		syscall 				# print the prompt string
		add $a0,$s3,0	
		li $v0, 11
		syscall
		la $a0, nline
		li $v0, 4
		syscall 				# print \n

pMode:	la $a0, msg5
		li $v0, 4
		syscall 				# print the prompt string
		add $a0,$s4,0	
		li $v0, 11
		syscall

# The program is finished. Exit.
		li   $v0, 10            # system call for exit
		syscall                 # Exit!


#### Print Subroutine ####
		.data
		.text
print:	li $t2, 0			# start position of count
		move $t4, $t1
		mulo $t4,$t4,2
PSA:	add $s0,$s0,$t2		# move address accordingly to the counter
		add $t2,$t2, 1		# increment counter
		lb $a0,($s0)	
		li $v0, 11
		syscall
		la $s0,buf1			# restores buf1 starting address
		blt $t2,$t4,PSA		# Branch 
		jr $ra              # return from subroutine

#### End Print ####
