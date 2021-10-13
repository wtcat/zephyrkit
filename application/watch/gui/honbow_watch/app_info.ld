SECTION_DATA_PROLOGUE(.app_node,,SUBALIGN(4))
{
  __app_node_start = .;
  KEEP(*(SORT(.app_node_*)));
  __app_node_end = .;
} GROUP_LINK_IN(ROMABLE_REGION)