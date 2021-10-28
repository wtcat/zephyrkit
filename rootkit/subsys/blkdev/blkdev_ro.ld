SECTION_DATA_PROLOGUE(.blkdev.partiton,,SUBALIGN(4))
{
  _k_blkdev_partition_list_start = .;
  KEEP(*(SORT(.k_blkdev_partition*)));
  _k_blkdev_partition_list_end = .;
} GROUP_LINK_IN(ROMABLE_REGION)