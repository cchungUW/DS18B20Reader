

/*-------------------------------------------------------------------------*/
/*--------------------------------------------------------------------------*/

/*
    $Id: gnuplot_i.c,v 2.10 2003/01/27 08:58:04 ndevilla Exp $
    $Author: ndevilla $
    $Date: 2003/01/27 08:58:04 $
    $Revision: 2.10 $
 */

/*---------------------------------------------------------------------------
                                Includes
 ---------------------------------------------------------------------------*/

#include "gnuplot_i.h"

#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <stdarg.h>
#include <assert.h>

#ifdef WIN32
#include <io.h>
#endif // #ifdef WIN32

/*---------------------------------------------------------------------------
                                Defines
 ---------------------------------------------------------------------------*/

/*---------------------------------------------------------------------------
                          Prototype Functions
 ---------------------------------------------------------------------------*/

char const * gnuplot_tmpfile(gnuplot_ctrl * handle);

void gnuplot_plot_atmpfile(gnuplot_ctrl * handle, char const* tmp_filename, char const* title);

/*---------------------------------------------------------------------------
                            Function codes
 ---------------------------------------------------------------------------*/

/*-------------------------------------------------------------------------*/
/*--------------------------------------------------------------------------*/

gnuplot_ctrl * gnuplot_init(void)
{
    gnuplot_ctrl *  handle ;
    int i;

#ifndef WIN32
    if (getenv("DISPLAY") == NULL) {
        fprintf(stderr, "cannot find DISPLAY variable: is it set?\n") ;
    }
#endif // #ifndef WIN32


    /*
     * Structure initialization:
     */
    handle = (gnuplot_ctrl*)malloc(sizeof(gnuplot_ctrl)) ;
    handle->nplots = 0 ;
    gnuplot_setstyle(handle, "points") ;
    handle->ntmp = 0 ;

    handle->gnucmd = popen("gnuplot", "w") ;
    if (handle->gnucmd == NULL) {
        fprintf(stderr, "error starting gnuplot, is gnuplot or gnuplot.exe in your path?\n") ;
        free(handle) ;
        return NULL ;
    }

    for (i=0;i<GP_MAX_TMP_FILES; i++)
    {
        handle->tmp_filename_tbl[i] = NULL;
    }
    return handle;
}


/*-------------------------------------------------------------------------*/
/*--------------------------------------------------------------------------*/

void gnuplot_close(gnuplot_ctrl * handle)
{
    int     i ;

    if (pclose(handle->gnucmd) == -1) {
        fprintf(stderr, "problem closing communication to gnuplot\n") ;
        return ;
    }
    if (handle->ntmp) {
        for (i=0 ; i<handle->ntmp ; i++) {
            remove(handle->tmp_filename_tbl[i]) ;
            free(handle->tmp_filename_tbl[i]);
            handle->tmp_filename_tbl[i] = NULL;

        }
    }
    free(handle) ;
    return ;
}


/*-------------------------------------------------------------------------*/
/*--------------------------------------------------------------------------*/

void gnuplot_cmd(gnuplot_ctrl *  handle, char const *  cmd, ...)
{
    va_list ap ;

    va_start(ap, cmd);
    vfprintf(handle->gnucmd, cmd, ap);
    va_end(ap);

    fputs("\n", handle->gnucmd) ;
    fflush(handle->gnucmd) ;
    return ;
}


/*-------------------------------------------------------------------------*/
/*--------------------------------------------------------------------------*/

void gnuplot_setstyle(gnuplot_ctrl * h, char * plot_style)
{
    if (strcmp(plot_style, "lines") &&
        strcmp(plot_style, "points") &&
        strcmp(plot_style, "linespoints") &&
        strcmp(plot_style, "impulses") &&
        strcmp(plot_style, "dots") &&
        strcmp(plot_style, "steps") &&
        strcmp(plot_style, "errorbars") &&
        strcmp(plot_style, "boxes") &&
        strcmp(plot_style, "boxerrorbars")) {
        fprintf(stderr, "warning: unknown requested style: using points\n") ;
        strcpy(h->pstyle, "points") ;
    } else {
        strcpy(h->pstyle, plot_style) ;
    }
    return ;
}


/*-------------------------------------------------------------------------*/
/*--------------------------------------------------------------------------*/

void gnuplot_set_xlabel(gnuplot_ctrl * h, char * label)
{
    gnuplot_cmd(h, "set xlabel \"%s\"", label) ;
}


/*-------------------------------------------------------------------------*/
/*--------------------------------------------------------------------------*/

void gnuplot_set_ylabel(gnuplot_ctrl * h, char * label)
{
    gnuplot_cmd(h, "set ylabel \"%s\"", label) ;
}


/*-------------------------------------------------------------------------*/
/*--------------------------------------------------------------------------*/

void gnuplot_resetplot(gnuplot_ctrl * h)
{
    int     i ;
    if (h->ntmp) {
        for (i=0 ; i<h->ntmp ; i++) {
            remove(h->tmp_filename_tbl[i]) ;
            free(h->tmp_filename_tbl[i]);
            h->tmp_filename_tbl[i] = NULL;

        }
    }
    h->ntmp = 0 ;
    h->nplots = 0 ;
    return ;
}

/*-------------------------------------------------------------------------*/
/*--------------------------------------------------------------------------*/

void gnuplot_plot_x(
    gnuplot_ctrl    *   handle,
    double          *   d,
    int                 n,
    char            *   title
)
{
    int     i ;
    FILE*   tmpfd ;
    char const * tmpfname;

    if (handle==NULL || d==NULL || (n<1)) return ;

    /* Open temporary file for output   */
    tmpfname = gnuplot_tmpfile(handle);
    tmpfd = fopen(tmpfname, "w");

    if (tmpfd == NULL) {
        fprintf(stderr,"cannot create temporary file: exiting plot") ;
        return ;
    }

    /* Write data to this file  */
    for (i=0 ; i<n ; i++) {
      fprintf(tmpfd, "%.18e\n", d[i]);
    }
    fclose(tmpfd) ;

    gnuplot_plot_atmpfile(handle,tmpfname,title);
    return ;
}



/*-------------------------------------------------------------------------*/
/*--------------------------------------------------------------------------*/

void gnuplot_plot_xy(
    gnuplot_ctrl    *   handle,
    double          *   x,
    double          *   y,
    int                 n,
    char            *   title
)
{
    int     i ;
    FILE*   tmpfd ;
    char const * tmpfname;

    if (handle==NULL || x==NULL || y==NULL || (n<1)) return ;

    /* Open temporary file for output   */
    tmpfname = gnuplot_tmpfile(handle);
    tmpfd = fopen(tmpfname, "w");

    if (tmpfd == NULL) {
        fprintf(stderr,"cannot create temporary file: exiting plot") ;
        return ;
    }

    /* Write data to this file  */
    for (i=0 ; i<n; i++) {
        fprintf(tmpfd, "%.18e %.18e\n", x[i], y[i]) ;
    }
    fclose(tmpfd) ;

    gnuplot_plot_atmpfile(handle,tmpfname,title);
    return ;
}



/*-------------------------------------------------------------------------*/
/*--------------------------------------------------------------------------*/

void gnuplot_plot_once(
  char    *   title,
  char    *   style,
  char    *   label_x,
  char    *   label_y,
  double  *   x,
  double  *   y,
  int         n
)
{
  gnuplot_ctrl    *   handle ;

  if (x==NULL || n<1) return ;

  if ((handle = gnuplot_init()) == NULL) return ;
  if (style!=NULL) {
      gnuplot_setstyle(handle, style);
  } else {
      gnuplot_setstyle(handle, "lines");
  }
  if (label_x!=NULL) {
      gnuplot_set_xlabel(handle, label_x);
  } else {
      gnuplot_set_xlabel(handle, "X");
  }
  if (label_y!=NULL) {
      gnuplot_set_ylabel(handle, label_y);
  } else {
      gnuplot_set_ylabel(handle, "Y");
  }
  if (y==NULL) {
      gnuplot_plot_x(handle, x, n, title);
  } else {
      gnuplot_plot_xy(handle, x, y, n, title);
  }
  printf("press ENTER to continue\n");
  while (getchar()!='\n') {}
  gnuplot_close(handle);
  return ;
}

void gnuplot_plot_slope(
    gnuplot_ctrl    *   handle,
    double              a,
    double              b,
    char            *   title
)
{
    char const *    cmd    = (handle->nplots > 0) ? "replot" : "plot";
    title                  = (title == NULL)      ? "(none)" : title;

    gnuplot_cmd(handle, "%s %.18e * x + %.18e title \"%s\" with %s",
                  cmd, a, b, title, handle->pstyle) ;

    handle->nplots++ ;
    return ;
}


void gnuplot_plot_equation(
    gnuplot_ctrl    *   h,
    char            *   equation,
    char            *   title
)
{
    char const *    cmd    = (h->nplots > 0) ? "replot" : "plot";
    title                  = (title == NULL)      ? "(none)" : title;

    gnuplot_cmd(h, "%s %s title \"%s\" with %s",
                  cmd, equation, title, h->pstyle) ;
    h->nplots++ ;
    return ;
}


int gnuplot_write_x_csv(
    char const * fileName,
    double const * d,
    int n,
    char const * title)
{
    int     i;
    FILE*   fileHandle;

    if (fileName==NULL || d==NULL || (n<1))
    {
        return -1;
    }

    fileHandle = fopen(fileName, "w");

    if (fileHandle == NULL)
    {
        return -1;
    }

    // Write Comment.
    if (title != NULL)
    {
        fprintf(fileHandle, "# %s\n", title) ;
    }

    /* Write data to this file  */
    for (i=0 ; i<n; i++)
    {
        fprintf(fileHandle, "%d, %.18e\n", i, d[i]) ;
    }

    fclose(fileHandle) ;

    return 0;
}

int gnuplot_write_xy_csv(
    char const *        fileName,
    double const    *   x,
    double const    *   y,
    int                 n,
    char const      *   title)
{
    int     i ;
    FILE*   fileHandle;

    if (fileName==NULL || x==NULL || y==NULL || (n<1))
    {
        return -1;
    }

    fileHandle = fopen(fileName, "w");

    if (fileHandle == NULL)
    {
        return -1;
    }

    // Write Comment.
    if (title != NULL)
    {
        fprintf(fileHandle, "# %s\n", title) ;
    }

    /* Write data to this file  */
    for (i=0 ; i<n; i++)
    {
        fprintf(fileHandle, "%.18e, %.18e\n", x[i], y[i]) ;
    }

    fclose(fileHandle) ;

    return 0;
}

int gnuplot_write_multi_csv(
    char const *        fileName,
    double const    **  xListPtr,
    int                 n,
    int                 numColumns,
    char const      *   title)
{
    int     i;
    int     j;
    FILE*   fileHandle;

    if (fileName==NULL || xListPtr==NULL || (n<1) || numColumns <1)
    {
        return -1;
    }

    for (j=0;j<numColumns;j++)
    {
        if (xListPtr[j] == NULL)
        {
            return -1;
        }
    }

    fileHandle = fopen(fileName, "w");

    if (fileHandle == NULL)
    {
        return -1;
    }

    // Write Comment.
    if (title != NULL)
    {
        fprintf(fileHandle, "# %s\n", title) ;
    }

    /* Write data to this file  */
    for (i=0 ; i<n; i++)
    {
        fprintf(fileHandle, "%d, %.18e", i, xListPtr[0][i]) ;
        for (j=1;j<numColumns;j++)
        {
            fprintf(fileHandle, ", %.18e", xListPtr[j][i]) ;
        }
        fprintf(fileHandle, "\n");
    }

    fclose(fileHandle) ;

    return 0;
}

char const * gnuplot_tmpfile(gnuplot_ctrl * handle)
{
    static char const * tmp_filename_template = "gnuplot_tmpdatafile_XXXXXX";
    char *              tmp_filename = NULL;
    int                 tmp_filelen = strlen(tmp_filename_template);

#ifndef WIN32
    int                 unx_fd;
#endif // #ifndef WIN32

    assert(handle->tmp_filename_tbl[handle->ntmp] == NULL);

    /* Open one more temporary file? */
    if (handle->ntmp == GP_MAX_TMP_FILES - 1) {
        fprintf(stderr,
                "maximum # of temporary files reached (%d): cannot open more",
                GP_MAX_TMP_FILES) ;
        return NULL;
    }

    tmp_filename = (char*) malloc(tmp_filelen+1);
    if (tmp_filename == NULL)
    {
        return NULL;
    }
    strcpy(tmp_filename, tmp_filename_template);

#ifdef WIN32
    if (_mktemp(tmp_filename) == NULL)
    {
        return NULL;
    }
#else // #ifdef WIN32
    unx_fd = mkstemp(tmp_filename);
    if (unx_fd == -1)
    {
        return NULL;
    }
    pclose(unx_fd);

#endif // #ifdef WIN32

    handle->tmp_filename_tbl[handle->ntmp] = tmp_filename;
    handle->ntmp ++;
    return tmp_filename;
}

void gnuplot_plot_atmpfile(gnuplot_ctrl * handle, char const* tmp_filename, char const* title)
{
    char const *    cmd    = (handle->nplots > 0) ? "replot" : "plot";
    title                  = (title == NULL)      ? "(none)" : title;
    gnuplot_cmd(handle, "%s \"%s\" title \"%s\" with %s", cmd, tmp_filename,
                  title, handle->pstyle) ;
    handle->nplots++ ;
    return ;
}


/* vim: set ts=4 et sw=4 tw=75 */