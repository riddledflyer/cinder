.section .rodata.pophappiness_ttf, "a"
.balign 4
.global pophappiness_ttf
pophappiness_ttf:
.incbin "../resources/pophappiness.ttf"
.global pophappiness_ttf_end
pophappiness_ttf_end:

.balign 4
.global pophappiness_ttf_size
pophappiness_ttf_size:
.int pophappiness_ttf_end - pophappiness_ttf
