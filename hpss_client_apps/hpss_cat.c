#include <stdio.h>
#include <errno.h>
#include <fcntl.h>
#include <sys/time.h>
#include <getopt.h>
#include "hpss_api.h"
#include "u_signed64.h"
#include "tools.h"

#define BUFSIZE (32*1024*1024)

main (int argc, char *argv[])
{
  int i, fd, hfd, rc, len;
  double start, elapsed, double_time (), xfer_rate;
  char *buf, *hpss_file, xfer_rate64_str[256], size64_str[256];
  char c;
  u_signed64 size64, count64;
  hpss_fileattr_t attr;

  while ((c = getopt (argc, argv, COMMON_OPTS)) != -1)
    {
      switch (c)
	{
	case 'h':
	case '?':
	  usage ();
	  exit (0);
	default:
	  if (scan_cmdline_authargs (c, optarg))
	    {
	      usage ();
	      exit (1);
	    }
	}
    }

  if (optind == argc)
    {
      usage ();
      exit (1);
    }
  hpss_file = argv[optind];
/*
 * Allocate the data buffer
 */
  buf = (char *) malloc (BUFSIZE);
  if (buf == NULL)
    {
      fprintf (stderr, "hpsscat: could not allocate memory\n");
      exit (0);
    }

  authenticate ();
/*
 * Open the HPSS source file
 */
  hfd = hpss_Open (hpss_file, O_RDONLY, 0, NULL, NULL, NULL);
  if (hfd < 0)
    {
      fprintf (stderr, "hpss_Open: ");
      perror (hpss_file);
      hpss_exit (0);
    }
/*
 * Get the length of the source file
 */
  if (hpss_FileGetAttributes (hpss_file, &attr) < 0)
    {
      fprintf (stderr, "hpss_FileGetAttributes: ");
      perror (hpss_file);
      hpss_exit (0);
    }
  size64 = attr.Attrs.DataLength;
/*
 * Loop through the source files
 */
  start = double_time ();
  count64 = cast64m (0);
  while (lt64m (count64, size64))
    {
/*
 * Read in the next chunk of data
 */
      len = hpss_Read (hfd, buf, BUFSIZE);
      if (len <= 0)
	{
	  perror ("hpss_Read");
	  hpss_exit (0);
	}
/*
 * Write the data to standard output
 */
      rc = fwrite (buf, 1, len, stdout);
      if (rc != len)
	{
	  perror ("fwrite");
	  hpss_exit (0);
	}
      count64 = add64m (count64, cast64m (len));
    }
/*
 * Close HPSS file and display transfer statistics
 */
  hpss_Close (hfd);
  elapsed = double_time () - start;
  xfer_rate = ((double) cast32m (div64m (size64, 1024 * 1024))) / elapsed;
  u64_to_decchar (size64, size64_str);
  fprintf (stderr, "%s: \"%s\" %s Bytes in %.3f sec -> %.3f MB/sec\n",
	   argv[0], hpss_file, size64_str, elapsed, xfer_rate);

  hpss_exit (0);
}

usage ()
{
  fprintf (stderr,
	   "prints contents of an hpss-file to STDOUT.\nUsage: hpsscat [auth-options] <srcfl>\n");
  common_usage();
}
