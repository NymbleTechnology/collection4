#include <stdlib.h>
#include <stdio.h>
#include <stdarg.h>
#include <string.h>
#include <errno.h>

#include "utils_array.h"

struct str_array_s
{
  char **ptr;
  size_t size;
};

static int sort_callback (const void *v0, const void *v1) /* {{{ */
{
  const char *c0 = v0;
  const char *c1 = v1;

  return (strcmp (c0, c1));
} /* }}} int sort_callback */

str_array_t *array_create (void) /* {{{ */
{
  str_array_t *a;

  a = malloc (sizeof (*a));
  if (a == NULL)
    return (NULL);

  memset (a, 0, sizeof (*a));
  a->ptr = NULL;
  a->size = 0;

  return (a);
} /* }}} str_array_t *array_create */

void array_destroy (str_array_t *a) /* {{{ */
{
  if (a == NULL)
    return;

  free (a->ptr);
  a->ptr = NULL;
  a->size = 0;

  free (a);
} /* }}} void array_destroy */

int array_append (str_array_t *a, const char *entry) /* {{{ */
{
  char **ptr;

  if ((entry == NULL) || (a == NULL))
    return (EINVAL);

  ptr = realloc (a->ptr, sizeof (*a->ptr) * (a->size + 1));
  if (ptr == NULL)
    return (ENOMEM);
  a->ptr = ptr;
  ptr = a->ptr + a->size;

  *ptr = strdup (entry);
  if (*ptr == NULL)
    return (ENOMEM);

  a->size++;
  return (0);
} /* }}} int array_append */

int array_append_format (str_array_t *a, const char *format, ...) /* {{{ */
{
  char buffer[1024];
  va_list ap;
  int status;

  va_start (ap, format);
  status = vsnprintf (buffer, sizeof (buffer), format, ap);
  va_end(ap);

  if ((status < 0) || (((size_t) status) >= sizeof (buffer)))
    return (ENOMEM);

  return (array_append (a, buffer));
} /* }}} int array_append_format */

int array_sort (str_array_t *a) /* {{{ */
{
  if (a == NULL)
    return (EINVAL);

  qsort (a->ptr, a->size, sizeof (*a->ptr), sort_callback);

  return (0);
} /* }}} int array_sort */

int array_argc (str_array_t *a) /* {{{ */
{
  if (a == NULL)
    return (-1);

  return ((int) a->size);
} /* }}} int array_argc */

char **array_argv (str_array_t *a) /* {{{ */
{
  if ((a == NULL) || (a->size == 0))
    return (NULL);

  return (a->ptr);
} /* }}} char **array_argv */

/* vim: set sw=2 sts=2 et fdm=marker : */
