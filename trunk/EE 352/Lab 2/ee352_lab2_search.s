# ee352_lab3.s
# EE 352, Spring 2008
# Name:  Niels-David Yogi
# Description: In this lab we will write a program to find a pathway through
# 			   a maze using a simple (brute-force) recursive (depth-first) 
#   		   search algorithm.
# Pre-req: Maze must be a square.

		.data
maze:   .byte 0,0,0,0,0
		.byte 1,1,1,1,0
		.byte 0,0,0,0,0
		.byte 0,1,1,1,1
		.byte 0,0,0,0,0

visit:  .byte 0,0,0,0,0
		.byte 0,0,0,0,0
		.byte 0,0,0,0,0
		.byte 0,0,0,0,0
		.byte 0,0,0,0,0

msize:	.word	5
msg_y:	.asciiz  "Path through maze EXISTS!!\n"
msg_n:	.asciiz  "Path through maze DOES NOT EXIST!!\n"
msg_p:  .asciiz  "Path from exit to entrance\n"
op_par: .asciiz  "("
comma:  .asciiz  ","
cl_par: .asciiz  ")"

# print this string for a newline character
nline:  .asciiz "\n"


		.text
####
#  main : Initializes pointers to maze and visited array, sets up initial r,c values
#         and calls search
#     
#       NOTE:  You should not have to change MAIN...It is complete!
#
####
main:   la		$s6, maze	# Base pointer to maze array
		la 		$s7, visit  # Base pointer to vis(ited) array
		li		$a0, 0		# r variable 
		li  	$a1, 0      # c variable
		jal		search
		bne		$v0,$zero,mL1
		la		$a0, msg_n
		li		$v0, 4
		syscall
		# exit
mL1:	li		$v0,10
		syscall
		
		
####
#  search : Recursive Maze search routine
#   $a0 = r
#   $a1 = c
#   returns $v0 => 1 = found a path from entrance to exit, 0 => No path found yet
####		
search:	addi    $sp,$sp,-4	# Move the stack pointer for Return Address
		sw		$ra,0($sp)	# Push Return Address onto the stack
		addi    $sp,$sp,-8	# Move the stack pointer for Arguments
		sw		$a1,4($sp)	# Push 'c' onto the stack
		sw		$a0,0($sp)	# Push 'r' onto the stack
		
		
 		# Check if invalid location
		jal iloc
		beq		$v0,$zero,sVisit	# If location is valid, continue with function
		b 		sNoPath				# If location is invalid, return 0

sVisit:	# Check if visited this location before
		jal vis
		beq		$v0,$zero,sExit		# If location is not visited, continue with function
		b 		sNoPath				# If location is visited, return 0

sExit:	# Check if at exit and path was found
		jal atexit
		beq		$v0,$zero,sEast		# If location is a not an exit, continue with function
									# If location is an exit,
		jal ploc					# Print '(r,c)'
		li 		$v0,1				# return 1
		b	sUnload		

sEast:	# Check to the east
		lw		$a1,4($sp)			# Restore c
		lw		$a0,0($sp)			# Restore r
		addi 	$a1,$a1,1			# c++
		jal search					# Recurse through function with new arguments (r,c++)
		beq 	$v0,$zero,sSouth	# If return value = 0, continue with function
									# If return value = 1,
		lw		$a1,4($sp)			# Restore c
		lw		$a0,0($sp)			# Restore r
		jal ploc					# Print '(r,c)'
		li 		$v0,1				# return 1
		b	sUnload

sSouth:	# Check to the south
		lw		$a1,4($sp)			# Restore c
		lw		$a0,0($sp)			# Restore r
		addi 	$a0,$a0,1			# r++
		jal search					# Recurse through function with new arguments (r++,c)
		beq 	$v0,$zero,sWest		# If return value = 0, continue with function
									# If return value = 1,
		lw		$a1,4($sp)			# Restore c
		lw		$a0,0($sp)			# Restore r
		jal ploc					# Print '(r,c)'
		li 		$v0,1				# return 1
		b	sUnload

sWest:	# Check to the west
		lw		$a1,4($sp)			# Restore c
		lw		$a0,0($sp)			# Restore r
		subi 	$a1,$a1,1			# c--
		jal search					# Recurse through function with new arguments (r,c--)
		beq 	$v0,$zero,sNorth	# If return value = 0, continue with function
									# If return value = 1,
		lw		$a1,4($sp)			# Restore c
		lw		$a0,0($sp)			# Restore r
		jal ploc					# Print '(r,c)'
		li 		$v0,1				# return 1
		b	sUnload

sNorth:	# Check to the north
		lw		$a1,4($sp)			# Restore c
		lw		$a0,0($sp)			# Restore r
		subi 	$a0,$a0,1			# r--
		jal search					# Recurse through function with new arguments (r--,c)
		beq 	$v0,$zero,sNoPath	# If return value = 0, continue with function
									# If return value = 1,
		lw		$a1,4($sp)			# Restore c
		lw		$a0,0($sp)			# Restore r
		jal ploc					# Print '(r,c)'
		li 		$v0,1				# return 1
		b	sUnload

sNoPath:# If no path was found, return false
		# Basically "return 0"
		li 		$v0,0				# return 0
		
sUnload:lw		$a0,0($sp)			# Restore r
		lw		$a1,4($sp)			# Restore c
		addi    $sp,$sp,8			# Pop r & c
		lw		$ra,0($sp)			# Restore return address
		addi    $sp,$sp,4			# Pop return address
		jr		$ra					# Jump back to specified return address

####
#  iloc = Invalid Location (either a wall or out of bounds)
#   $a0 = r
#   $a1 = c
#   returns $v0 => 1 = invalid, 0 = valid
####
iloc:   blt 	$a0,$zero,inValid	# If r < 0, return 1
		blt 	$a1,$zero,inValid	# Or if c < 0, return 1
		lw		$t0,msize			# Get value of msize (size of maze)
		subi 	$t0,$t0,1			# msize--
		bgt 	$a0,$t0,inValid		# Or if r > msize--, return 1
		bgt 	$a1,$t0,inValid		# Or if c > msize--, return 1
									# Else, return maze[r][c]
		lw	 	$t1,msize			# Get value of msize (size of maze)
		mulo 	$t1,$a0,$t1			# t1 = r * msize
		add	 	$t1,$t1,$a1			# t1 += c
		lb 		$t2, maze($t1)		# t2 = maze[r][c]
		addi 	$v0,$t2,0			# v0 = t2
		b		iJump

inValid:li	 	$v0,1				# return 1
iJump:	jr		$ra					# Jump back to specified return address

####
#  ATEXIT = At Exit?
#   $a0 = r
#   $a1 = c
#   returns $v0 => 1 = at exit, 0 = not at exit
#     and prints success message
####
atexit:	lw	 	$t0,msize			# Get value of msize (size of maze)
		subi	$t0,$t0,1			# msize--
		bne 	$a0,$t0,aNotXit		# If r != msize--, return 0 
		bne 	$a1,$t0,aNotXit		# Or if c != msize--, return 0
									# Else, 
		addi	$t0,$a0,0			# Save r
		la		$a0, msg_y				
		li		$v0, 4
		syscall
		li		$v0, 1				# return 1
		addi 	$a0,$t0,0			# Restore r
		b		aJump

aNotXit:li		$v0, 0				# return 0
aJump:	jr		$ra					# Jump back to specified return address

####
#  vis = Visited?
#   $a0 = r
#   $a1 = c
#   returns $v0 => 1 = locations has already been visited, 0 = not visited 
#     and marks location as visited
####
vis: 	lw	 	$t0,msize			# Get value of msize (size of maze)
		mulo 	$t1,$a0,$t0			# t1 = r * msize
		add	 	$t1,$t1,$a1			# t1 += c
		lb 		$t2, visit($t1)		# t2 = visit[r][c]
		li		$t3,1 
		sb		$t3, visit($t1)		# visit[r][c] = 1

		addi 	$v0,$t2,0			# v0 = t2 or v0 = visit[r][c]
		jr		$ra					# Jump back to specified return address

####
#  ploc = Print Location
#   $a0 = r
#   $a1 = c
####
ploc:	addi 	$t0,$a0,0			# Save r
		addi 	$t1,$a1,0			# Save c
		la		$a0, op_par			
		li		$v0, 4
		syscall						# Print (
		addi	$a0, $t0,0
		li		$v0, 1
		syscall						# Print r
		la		$a0, comma
		li		$v0, 4
		syscall						# Print ,
		addi	$a0, $t1,0
		li		$v0, 1
		syscall						# Print c
		la		$a0, cl_par
		li		$v0, 4
		syscall						# Print )

		addi 	$a0,$t0,0			# Restore r
		addi 	$a1,$t1,0			# Restore c
		jr		$ra					# Jump back to specified return address



