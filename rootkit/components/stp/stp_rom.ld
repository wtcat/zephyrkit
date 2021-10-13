	
SECTION_DATA_PROLOGUE(.stp_node,,SUBALIGN(4))
{
  __stp_node_start = .;
  KEEP(*(SORT(.stp_node_*)));
  __stp_node_end = .;
} GROUP_LINK_IN(ROMABLE_REGION)
