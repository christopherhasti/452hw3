#ifndef ERROR_H
#define ERROR_H

/* Print an error message and exit */
extern void ERROR(const char *fmt, ...);

/* Print an error message with file and line location, then exit */
extern void ERRORLOC(const char *file, int line, const char *type, const char *fmt, ...);

#endif