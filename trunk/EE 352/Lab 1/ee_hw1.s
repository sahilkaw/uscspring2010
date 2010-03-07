.data
msg: .byte 1, 2 ,3
.text

#li $t0,0x7fffabcd
#lui $1,0x7fff
#ori $8, $1,0xabcd
#bgt $t1,$t2,L1

#L1:
li $t3, 0x30
li $t0, 0x68ce8932
li $t1, 0x4ffd3447
sub $t2, $t0,$t1 
sll $t2,$t2,1
add $t3,$t3,$t2
