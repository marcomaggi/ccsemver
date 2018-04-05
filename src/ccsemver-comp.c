/*
  Part of: CCSemver
  Contents: parsing comparators and creating comparator objects


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
#include <stdlib.h>
#include <stdio.h>


/** --------------------------------------------------------------------
 ** Constructor and destructor.
 ** ----------------------------------------------------------------- */

void
ccsemver_comp_ctor (ccsemver_comp_t *self)
{
  self->next	= NULL;
  self->op	= CCSEMVER_OP_EQ;
  ccsemver_ctor(&self->version);
}

void
ccsemver_comp_dtor (ccsemver_comp_t * self)
{
  if (self->next) {
    ccsemver_comp_dtor(self->next);
    free(self->next);
    self->next = NULL;
  }
}


/** --------------------------------------------------------------------
 ** Parser.
 ** ----------------------------------------------------------------- */

static void ccsemver_xrevert (ccsemver_t * semver);
static ccsemver_comp_t * ccsemver_xconvert (ccsemver_comp_t * self);
static char parse_partial (ccsemver_t * self, char const * str, size_t len, size_t * offset);
static char parse_hiphen (ccsemver_comp_t * self, char const * str, size_t len, size_t * offset);
static char parse_tilde (ccsemver_comp_t * self, char const * str, size_t len, size_t * offset);
static char parse_caret (ccsemver_comp_t * self, char const * str, size_t len, size_t * offset);

char
ccsemver_comp_read (ccsemver_comp_t * self, char const * str, size_t len, size_t * offset)
{
  ccsemver_comp_ctor(self);
  while (*offset < len) {
    switch (str[*offset]) {
    case '^':
      ++*offset;
      if (parse_tilde(self, str, len, offset)) {
	return 1;
      }
      self = self->next;
      goto next;
    case '~':
      ++*offset;
      if (parse_caret(self, str, len, offset)) {
	return 1;
      }
      self = self->next;
      goto next;
    case '>':
      ++*offset;
      if (*offset < len && str[*offset] == '=') {
	++*offset;
	self->op = CCSEMVER_OP_GE;
      } else {
	self->op = CCSEMVER_OP_GT;
      }
      if (parse_partial(&self->version, str, len, offset)) {
	return 1;
      }
      ccsemver_xrevert(&self->version);
      goto next;
    case '<':
      ++*offset;
      if (*offset < len && str[*offset] == '=') {
	++*offset;
	self->op = CCSEMVER_OP_LE;
      } else {
	self->op = CCSEMVER_OP_LT;
      }
      if (parse_partial(&self->version, str, len, offset)) {
	return 1;
      }
      ccsemver_xrevert(&self->version);
      goto next;
    case '=':
      ++*offset;
      self->op = CCSEMVER_OP_EQ;
      if (parse_partial(&self->version, str, len, offset)) {
	return 1;
      }
      ccsemver_xrevert(&self->version);
      goto next;
    default:
      goto range;
    }
  }

 range:
  if (parse_partial(&self->version, str, len, offset)) {
    return 1;
  }
  if (*offset < len && str[*offset] == ' '
      && *offset + 1 < len && str[*offset + 1] == '-'
      && *offset + 2 < len && str[*offset + 2] == ' ') {
    *offset += 3;
    if (parse_hiphen(self, str, len, offset)) {
      return 1;
    }
    self = self->next;
  } else {
    self = ccsemver_xconvert(self);
    if (self == NULL) {
      return 1;
    }
  }

 next:
  if (*offset < len && str[*offset] == ' '
      && *offset < len + 1 && str[*offset + 1] != ' ' && str[*offset + 1] != '|') {
    ++*offset;
    if (*offset < len) {
      self->next = (ccsemver_comp_t *) malloc(sizeof(ccsemver_comp_t));
      if (self->next == NULL) {
        return 1;
      }
      return ccsemver_comp_read(self->next, str, len, offset);
    }
    return 1;
  }
  return 0;
}


static void
ccsemver_xrevert (ccsemver_t * semver)
{
  if (CCSEMVER_NUM_X == semver->major) {
    semver->major = semver->minor = semver->patch = 0;
  } else if (CCSEMVER_NUM_X == semver->minor) {
    semver->minor = semver->patch = 0;
  } else if (CCSEMVER_NUM_X == semver->patch) {
    semver->patch = 0;
  }
}


static ccsemver_comp_t *
ccsemver_xconvert (ccsemver_comp_t * self)
{
  if (CCSEMVER_NUM_X == self->version.major) {
    self->op = CCSEMVER_OP_GE;
    ccsemver_xrevert(&self->version);
    return self;
  }
  if (CCSEMVER_NUM_X == self->version.minor) {
    ccsemver_xrevert(&self->version);
    self->op = CCSEMVER_OP_GE;
    self->next = (ccsemver_comp_t *) malloc(sizeof(ccsemver_comp_t));
    if (self->next == NULL) {
      return NULL;
    }
    ccsemver_comp_ctor(self->next);
    self->next->op = CCSEMVER_OP_LT;
    self->next->version = self->version;
    ++self->next->version.major;
    return self->next;
  }
  if (CCSEMVER_NUM_X == self->version.patch) {
    ccsemver_xrevert(&self->version);
    self->op = CCSEMVER_OP_GE;
    self->next = (ccsemver_comp_t *) malloc(sizeof(ccsemver_comp_t));
    if (self->next == NULL) {
      return NULL;
    }
    ccsemver_comp_ctor(self->next);
    self->next->op = CCSEMVER_OP_LT;
    self->next->version = self->version;
    ++self->next->version.minor;
    return self->next;
  }
  self->op = CCSEMVER_OP_EQ;
  return self;
}


static char
parse_partial (ccsemver_t * self, char const * str, size_t len, size_t * offset)
{
  ccsemver_ctor(self);
  self->major = self->minor = self->patch = CCSEMVER_NUM_X;
  if (*offset < len) {
    self->raw = str + *offset;
    if  (ccsemver_num_parse(&self->major, str, len, offset)) {
      return 0;
    }
    if (*offset >= len || str[*offset] != '.') {
      return 0;
    }
    ++*offset;
    if  (ccsemver_num_parse(&self->minor, str, len, offset)) {
      return 1;
    }
    if (*offset >= len || str[*offset] != '.') {
      return 0;
    }
    ++*offset;
    if  (ccsemver_num_parse(&self->patch, str, len, offset)) {
      return 1;
    }
    if ((str[*offset] == '-' && ccsemver_id_read(&self->prerelease, str, len, (++*offset, offset)))
	|| (str[*offset] == '+' && ccsemver_id_read(&self->build, str, len, (++*offset, offset)))) {
      return 1;
    }
    self->len = str + *offset - self->raw;
    return 0;
  }
  return 0;
}


static char
parse_hiphen (ccsemver_comp_t * self, char const * str, size_t len, size_t * offset)
{
  ccsemver_t partial;

  if (parse_partial(&partial, str, len, offset)) {
    return 1;
  }
  self->op = CCSEMVER_OP_GE;
  ccsemver_xrevert(&self->version);
  self->next = (ccsemver_comp_t *) malloc(sizeof(ccsemver_comp_t));
  if (self->next == NULL) {
    return 1;
  }
  ccsemver_comp_ctor(self->next);
  self->next->op = CCSEMVER_OP_LT;
  if (CCSEMVER_NUM_X == partial.minor) {
    self->next->version.major = partial.major + 1;
  } else if (CCSEMVER_NUM_X == partial.patch) {
    self->next->version.major = partial.major;
    self->next->version.minor = partial.minor + 1;
  } else {
    self->next->op = CCSEMVER_OP_LE;
    self->next->version = partial;
  }

  return 0;
}


static char
parse_tilde (ccsemver_comp_t * self, char const * str, size_t len, size_t * offset)
{
  ccsemver_t	partial;

  if (parse_partial(&self->version, str, len, offset)) {
    return 1;
  }
  ccsemver_xrevert(&self->version);
  self->op = CCSEMVER_OP_GE;
  partial = self->version;
  if (partial.major != 0) {
    ++partial.major;
    partial.minor = partial.patch = 0;
  } else if (partial.minor != 0) {
    ++partial.minor;
    partial.patch = 0;
  } else {
    ++partial.patch;
  }
  self->next = (ccsemver_comp_t *) malloc(sizeof(ccsemver_comp_t));
  if (self->next == NULL) {
    return 1;
  }
  ccsemver_comp_ctor(self->next);
  self->next->op = CCSEMVER_OP_LT;
  self->next->version = partial;
  return 0;
}


static char
parse_caret (ccsemver_comp_t * self, char const * str, size_t len, size_t * offset)
{
  ccsemver_t	partial;

  if (parse_partial(&self->version, str, len, offset)) {
    return 1;
  }
  ccsemver_xrevert(&self->version);
  self->op = CCSEMVER_OP_GE;
  partial = self->version;
  if (partial.minor || partial.patch) {
    ++partial.minor;
    partial.patch = 0;
  } else {
    ++partial.major;
    partial.minor = partial.patch = 0;
  }
  self->next = (ccsemver_comp_t *) malloc(sizeof(ccsemver_comp_t));
  if (self->next == NULL) {
    return 1;
  }
  ccsemver_comp_ctor(self->next);
  self->next->op = CCSEMVER_OP_LT;
  self->next->version = partial;
  return 0;
}


/** --------------------------------------------------------------------
 ** Composing comparators.
 ** ----------------------------------------------------------------- */

char
ccsemver_comp_and (ccsemver_comp_t * cmp, char const * str, size_t len, size_t * offset)
{
  if (0 < len) {
    ccsemver_comp_t *	new;

    /* Read a comparator from the input string. */
    {
      new = (ccsemver_comp_t *) malloc(sizeof(ccsemver_comp_t));
      if (NULL == new) {
	return 1;
      } else {
	char rv = ccsemver_comp_read(new, str, len, offset);
	if (rv) {
	  ccsemver_comp_dtor(new);
	  free(new);
	  return rv;
	}
      }
    }

    /* Append the new comparator to the existing CMP. */
    {
      if (NULL == cmp->next) {
	cmp->next = new;
      } else {
	ccsemver_comp_t * tail;

	/* Find the last comparator in the linked list CMP. */
	for (tail = cmp->next; tail->next; tail = tail->next);

	/* Append the new comparator to the linked list. */
	tail->next = new;
      }
    }

    return 0;
  } else {
    return 1;
  }
}


/** --------------------------------------------------------------------
 ** Matching comparators and versions.
 ** ----------------------------------------------------------------- */

char
ccsemver_match (ccsemver_t const * sv, ccsemver_comp_t const * cmp)
{
  char result = ccsemver_comp(sv, &(cmp->version));

  /* If "SV < CMP->VERSION" and the operator is neither "<" nor "<=": no
     match. */
  if ((result < 0) && (cmp->op != CCSEMVER_OP_LT) && (cmp->op != CCSEMVER_OP_LE)) {
    return 0;
  }

  /* If "SV > CMP->VERSION" and the operator is neither ">" nor ">=": no
     match. */
  if ((result > 0) && (cmp->op != CCSEMVER_OP_GT) && (cmp->op != CCSEMVER_OP_GE)) {
    return 0;
  }

  /* If "SV  == CMP->VERSION" and the  operator is neither "="  nor "<="
     nor ">=": no match. */
  if ((result == 0) && (cmp->op != CCSEMVER_OP_EQ) && (cmp->op != CCSEMVER_OP_LE) && (cmp->op != CCSEMVER_OP_GE)) {
    return 0;
  }

  /* If there  is a match between  SV and CMP->VERSION: go  on and match
     the next comparator (if any). */
  if (cmp->next) {
    return ccsemver_match(sv, cmp->next);
  } else {
    return 1;
  }
}


/** --------------------------------------------------------------------
 ** Serialisation.
 ** ----------------------------------------------------------------- */

int
ccsemver_comp_write (ccsemver_comp_t const * self, char * buffer, size_t len)
{
  char const *	op = "";
  char		semver[256];
  char		next[1024];

  switch (self->op) {
  case CCSEMVER_OP_EQ:
    break;
  case CCSEMVER_OP_LT:
    op = "<";
    break;
  case CCSEMVER_OP_LE:
    op = "<=";
    break;
  case CCSEMVER_OP_GT:
    op = ">";
    break;
  case CCSEMVER_OP_GE:
    op = ">=";
    break;
  }
  ccsemver_write(&(self->version), semver, 256);
  if (self->next) {
    return snprintf(buffer, len, "%s%s %.*s", op, semver, ccsemver_comp_write(self->next, next, 1024), next);
  } else {
    return snprintf(buffer, len, "%s%s", op, semver);
  }
}

/* end of file */
