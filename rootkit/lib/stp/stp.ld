	
SECTION_DATA_PROLOGUE(.stp_node,,SUBALIGN(4))
{
  __stp_node_start = .;
  KEEP(*(SORT(.stp_node_*)));
  __stp_node_end = .;
} GROUP_DATA_LINK_IN(RAMABLE_REGION, ROMABLE_REGION)
