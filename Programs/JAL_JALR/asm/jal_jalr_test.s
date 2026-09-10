# ===================================================
# Test JAL / JALR
# a0(x10) = resultat affiche
# t1(x6)  = adresse de retour (JAL avec rd != ra, pour prouver rd generique)
# t2(x7)  = doit rester a 0 (instruction sautee par le JALR +4)
# t3(x28) = doit valoir 111 (preuve que le saut +4 a fonctionne)
# ===================================================

addi x10, x0, 5         # a0 = 5

jal  x6, double_it        # appel : t1 = adresse de retour (PC+4), rd != ra volontairement
addi x7, x0, 999           # DOIT ETRE SAUTEE par le retour avec offset +4
addi x28, x0, 111          # DOIT ETRE EXECUTEE (preuve du saut +4 reussi)

jal  x0, end                # saute par-dessus le corps de la fonction
double_it:
    slli x10, x10, 1          # a0 = a0*2 = 10
    jalr x0, x6, 4              # retour avec offset +4 (pas +0), sauta l'instruction suivante

end:
    jal x0, end
