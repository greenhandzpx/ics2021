def_EHelper(jal) {
  rtl_jal(s, s->pc+id_src1->imm, ddest);
}

def_EHelper(ret) {
  rtl_jr(s, id_src1->preg);
}