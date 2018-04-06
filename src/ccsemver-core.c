/*
  Part of: CCSemver
  Contents: parser functions for semvers

  Abstract

	Parser functions for semvers.

  This  is  free and  unencumbered  software  released into  the  public
  domain.

  Anyone  is free  to  copy,  modify, publish,  use,  compile, sell,  or
  distribute this software, either in source  code form or as a compiled
  binary,  for any  purpose, commercial  or non-commercial,  and by  any
  means.

  In jurisdictions that recognize copyright  laws, the author or authors
  of  this software  dedicate  any  and all  copyright  interest in  the
  software  to the  public  domain.   We make  this  dedication for  the
  benefit of the public  at large and to the detriment  of our heirs and
  successors.   We  intend  this  dedication  to  be  an  overt  act  of
  relinquishment in perpetuity of all  present and future rights to this
  software under copyright law.

  THE  SOFTWARE IS  PROVIDED  "AS  IS", WITHOUT  WARRANTY  OF ANY  KIND,
  EXPRESS OR  IMPLIED, INCLUDING  BUT NOT LIMITED  TO THE  WARRANTIES OF
  MERCHANTABILITY, FITNESS FOR A PARTICULAR PURPOSE AND NONINFRINGEMENT.
  IN NO  EVENT SHALL  THE AUTHORS  BE LIABLE FOR  ANY CLAIM,  DAMAGES OR
  OTHER LIABILITY, WHETHER IN AN  ACTION OF CONTRACT, TORT OR OTHERWISE,
  ARISING FROM, OUT OF OR IN CONNECTION  WITH THE SOFTWARE OR THE USE OR
  OTHER DEALINGS IN THE SOFTWARE.

  For more information, please refer to <http://unlicense.org>
*/


/** --------------------------------------------------------------------
 ** Headers.
 ** ----------------------------------------------------------------- */

#include <ccsemver-internals.h>
#include <string.h>
#include <stdio.h>


/** --------------------------------------------------------------------
 ** Constructors and destructors.
 ** ----------------------------------------------------------------- */

void
ccsemver_ctor (ccsemver_t * self)
{
  self->len = 0;
  self->raw = NULL;
  self->major = 0;
  self->minor = 0;
  self->patch = 0;
  ccsemver_id_ctor(&self->prerelease);
  ccsemver_id_ctor(&self->build);
}

void
ccsemver_dtor (ccsemver_t * self)
{
  ccsemver_id_dtor(&self->prerelease);
  ccsemver_id_dtor(&self->build);
}


/** --------------------------------------------------------------------
 ** Parser.
 ** ----------------------------------------------------------------- */

char
ccsemver_read (ccsemver_t * self, ccsemver_input_t * input)
{
  if (ccsemver_input_more(input)) {
    ccsemver_ctor(self);
    self->raw = input->str + input->off;

    /* Skip the initial "v" character, if any. */
    if ('v' == ccsemver_input_next(input)) {
      ++(input->off);
    }

    if (ccsemver_parse_number(&self->major, input)
	|| input->off >= input->len || ccsemver_input_next(input) != '.'
	|| ccsemver_parse_number(&self->minor, (++input->off, input))
	|| input->off >= input->len || ccsemver_input_next(input) != '.'
	|| ccsemver_parse_number(&self->patch, (++input->off, input))
	|| (ccsemver_input_next(input) == '-' && ccsemver_id_read(&self->prerelease, (++input->off, input)))
	|| (ccsemver_input_next(input) == '+' && ccsemver_id_read(&self->build,      (++input->off, input)))) {
      self->len = input->str + input->off - self->raw;
      return 1;
    }
    self->len = input->str + input->off - self->raw;
    return 0;
  }
  return 1;
}


/** --------------------------------------------------------------------
 ** Comparision.
 ** ----------------------------------------------------------------- */

char
ccsemver_comp (ccsemver_t const * self, ccsemver_t const * other)
{
  char result;

  if (0 != (result = ccsemver_num_comp(self->major, other->major))) {
    return result;
  }
  if (0 != (result = ccsemver_num_comp(self->minor, other->minor))) {
    return result;
  }
  if (0 != (result = ccsemver_num_comp(self->patch, other->patch))) {
    return result;
  }
  if (0 != (result = ccsemver_id_comp(&(self->prerelease), &(other->prerelease)))) {
    return result;
  }
  return ccsemver_id_comp(&(self->build), &(other->build));
}


/** --------------------------------------------------------------------
 ** Serialisation.
 ** ----------------------------------------------------------------- */

int
ccsemver_write (ccsemver_t const * self, char *buffer, size_t len)
{
  char	prerelease[256], build[256];
  int	rv;

  if (self->prerelease.len && self->build.len) {
    rv = snprintf(buffer, len, "%ld.%ld.%ld-%.*s+%.*s",
		  self->major, self->minor, self->patch,
		  ccsemver_id_write(&(self->prerelease), prerelease, 256), prerelease,
		  ccsemver_id_write(&(self->build), build, 256), build);
  } else if (self->prerelease.len) {
    rv = snprintf(buffer, len, "%ld.%ld.%ld-%.*s",
		  self->major, self->minor, self->patch,
		  ccsemver_id_write(&(self->prerelease), prerelease, 256), prerelease);
  } else if (self->build.len) {
    rv = snprintf(buffer, len, "%ld.%ld.%ld+%.*s",
		  self->major, self->minor, self->patch,
		  ccsemver_id_write(&(self->build), build, 256), build);
  } else {
    rv = snprintf(buffer, len, "%ld.%ld.%ld", self->major, self->minor, self->patch);
  }
  return rv;
}

/* end of file */
