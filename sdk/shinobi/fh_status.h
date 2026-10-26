/* flash history safe refresh status strings; each object that includes
 * this header holds its own copy */
#ifndef FH_STATUS_H
#define FH_STATUS_H

/* the strings fhSetStatus writes for its four statuses. fh_ok marks the refresh
 * complete, and fh_exist the history's copy saved in the spare partition.
 * fh_process1 marks that copy in progress, and fh_process2 the spare partition
 * being written back */
static const char fh_ok[] = "SEGA_FH01_OK";
static const char fh_exist[] = "SEGA_FH01_EXIST";
static const char fh_process1[] = "SEGA_FH01_PROCESS1";
static const char fh_process2[] = "SEGA_FH01_PROCESS2";

#endif
