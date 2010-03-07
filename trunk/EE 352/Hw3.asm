.data
VALS: .word 16
.text

la $t0, VALS
lw $t1, ($t0)
srl $t1,$t1,1
#sll $t1,$t1,16
