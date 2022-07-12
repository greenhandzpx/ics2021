def_EHelper(lui) {
  rtl_li(s, ddest, id_src1->imm);
}

def_EHelper(li) {
  rtl_li(s, ddest, id_src2->imm);
}

def_EHelper(addi) {
  rtl_addi(s, ddest, id_src2->preg, id_src1->imm);
}


def_EHelper(auipc) {
  rtl_auipc(s, ddest, id_src1->imm);
}
