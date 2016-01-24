// grammar for mappings from joystick parameters to MIDI event types

%{
// forward declarations for bison
int yylex(void);
void yyerror(char const *);
// define the semantic value union
typedef union YYSTYPE YYSTYPE;
union YYSTYPE {
  int NUM;
  int type;
  MapSpec spec;
};
# define YYSTYPE_IS_DECLARED 1
// enable use of the undocumented yytoknum array for mapping keywords
static void print_token_value(FILE *, int, YYSTYPE);
#define YYPRINT(stream, token, value) \
  print_token_value(stream, token, value)
  
%}

%define parse.trace
%define parse.error verbose
%define api.value.type union

// a sequence of digits
%token <int> NUM
// a number that can be positive or negative
%type  <int> number
// event types
%token <int> AXIS "axis"
%token <int> BUTTON "button"
%token <int> NOTE "note"
%token <int> CONTROL "control"
%token <int> BEND "bend"
%token <int> IGNORE "ignore"
// parameters
%token <int> VERBOSITY "verbosity"
%token <int> DEBOUNCE "debounce"
%token <int> CHANNEL "channel"
// groupings
%type <MapSpec>	inspec
%type <MapSpec> outspec
%type <int>	joytype
%type <int>	miditype
%token-table

%% // grammar rules and actions

map:
  %empty
| map line
;

line:
  '\n'
| parameter '\n'
| mapping '\n'
;

number:
  NUM     { $$ = $1; }
| '-' NUM { $$ = - $2; }
;

parameter:
  VERBOSITY '=' number { verbosity = $3; }
| DEBOUNCE '=' number  { debounce = $3; }
| CHANNEL '=' number { channel = (($3 + 1) & 0xF); }
;

mapping:
  inspec '=' '>' outspec { add_mapping($1, $4); }
;

inspec:
  joytype number { 
    $$.type = $1;
    $$.number = $2;
    if ($1 == AXIS) {
      $$.min = -32767;
      $$.max = 32767;
    }
    else if ($1 == BUTTON) {
      $$.min = 0;
      $$.max = 1;
    }
  }
| joytype number '[' number ']' {
    $$.type = $1;
    $$.number = $2;
    $$.min = $4;
    $$.max = $4; 
  }
| joytype number '[' number '.' '.' number ']' {
    $$.type = $1; 
    $$.number = $2; 
    $$.min = $4; 
    $$.max = $7;
  }
;

joytype:
  AXIS
| BUTTON
;

outspec:
  IGNORE { $$.type = $1; }
| BEND {
    $$.type = $1;
    $$.min = 0x0000;
    $$.max = 0x3FFF;
  }
| BEND '[' NUM ']' {
    $$.type = $1;
    $$.min = $3;
    $$.max = $3;
  }
| BEND '[' NUM '.' '.' NUM ']' {
    $$.type = $1;
    $$.min = $3;
    $$.max = $6;
  }
| miditype NUM {
    $$.type = $1;
    $$.number = $2;
    $$.min = 0;
    $$.max = 127;
  }
| miditype NUM '[' NUM ']' {
    $$.type = $1;
    $$.number = $2;
    $$.min = $4;
    $$.max = $4;
  }
| miditype NUM '[' NUM '.' '.' NUM ']' {
    $$.type = $1;
    $$.number = $2;
    $$.min = $4;
    $$.max = $7;
  }
;

miditype:
  NOTE
| CONTROL
;

%%

// parse the map file
void parse_map() {
  // initialize the location structure
  yylloc.first_line = yylloc.last_line = 1;
  yylloc.first_column = yylloc.last_column = 0;
  // parse
  yyparse();
}

// get the next character from the map file
unsigned char nextchar() {
  ++yylloc.last_column;
  // check for the end of the file so we don't keep reading
  int c = fgetc(map_file);
  if (c < 0) c = 0;
  return(c);
}
// backtrack in the map file by the given number of characters
void backtrack(int count) {
  yylloc.last_column -= count;
  fseek(map_file, - count, SEEK_CUR);
}

int yylex(void) {
  int i, kwlen;
  char kwbuf[16];
  char kwc;
  unsigned char c = nextchar();
  // ignore whitespace
  while ((c == ' ') || (c == '\t')) {
    c = nextchar();
  }
  // ignore comments
  if (c == '#') {
    while ((c != '\n') && (c != 0)) {
      c = nextchar();
    }
  }
  // record the start position of the token
  yylloc.first_line = yylloc.last_line;
  yylloc.first_column = yylloc.last_column;
  // parse numbers
  if (isdigit(c)) {
    yylval.NUM = 0;
    while (isdigit(c)) {
      yylval.NUM = (yylval.NUM * 10) + (c - '0');
      c = nextchar();
    }
    backtrack(1);
    return(NUM);
  }
  // parse keywords
  if (isalpha(c)) {
    kwbuf[0] = c;
    i = 0;
    for (i = 1; i < 15; i++) {
      kwc = nextchar();
      kwbuf[i] = kwc;
      if (! isalpha(kwc)) {
        backtrack(1);
        break;
      }
    }
    kwbuf[i] = '\0';
    kwlen = i;
    for (i = 0; i < YYNTOKENS; i++) {
      if ((yytname[i] != 0) && (yytname[i][0] == '"') && 
          (! strncmp(yytname[i] + 1, kwbuf, kwlen)) &&
          (yytname[i][kwlen + 1] == '"') &&
          (yytname[i][kwlen + 2] == '\0')) {
        yylval.type = yytoknum[i];
        return(yytoknum[i]);
      }
    }
    backtrack(kwlen);
  }
  // advance the line counter
  if (c == '\n') {
    ++yylloc.last_line;
    yylloc.last_column = 0;
  }
  // all other characters are single-character tokens
  return(c);
}

void yyerror(char const *s) {
  int i;
  // write the error message
  fprintf(stderr, "ERROR: in %s line %d: %s\n", map_path, yylloc.first_line, s);
  // write the relevant line of the file to the console for reference,
  //  just like GCC does it
  char linebuf[1024];
  fseek(map_file, 0, SEEK_SET);
  int c = 0;
  int line = 1;
  while ((c >= 0) && (line < yylloc.first_line)) {
    c = fgetc(map_file);
    if (c == '\n') line++;
  }
  if (line == yylloc.first_line) {
    for (i = 0; i < sizeof(linebuf); i++) {
      c = fgetc(map_file);
      if (c == '\n') {
        linebuf[i] = '\0';
        break;
      }
      linebuf[i] = c;
    }
    if ((i < sizeof(linebuf)) && (i >= yylloc.first_column)) {
      fprintf(stderr, "%s\n", linebuf);
      for (i = 0; i < (yylloc.first_column - 1); i++) {
        fprintf(stderr, " ");
      }
      fprintf(stderr, "^\n");
    }
  }
}

static void print_token_value (FILE *file, int type, YYSTYPE value) {
  if (type == NUM) fprintf(file, "%d", value.NUM);
  else fprintf(file, "[%d]", type);
}
