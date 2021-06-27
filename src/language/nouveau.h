#ifndef Nouveau_h
#define Nouveau_h

#include <threads.h>
#include <stddef.h>
#include <stdbool.h>
#include <ctype.h>
#include <stdio.h>

#define NOUVEAU_REGISTERS     4
#define NOUVEAU_THREADS       7
#define NOUVEAU_BUFFER_SIZE   1024
#define NOUVEAU_SYMBOL_SIZE   64

struct Nou_Graph;

typedef struct Nou_Graph {
  void * functor;
  struct Nou_Graph * head;
  struct Nou_Graph * body;
  uint8_t type;
} nou_graph_t;

// Graph
#define nou_symbol_nil                '_'
#define nou_symbol_open_block         '['
#define nou_symbol_close_block        ']'
#define nou_symbol_open_scope         '{'
#define nou_symbol_close_scope        '}'
#define nou_symbol_graph_jump         ','
#define nou_symbol_graph_end          ';'
#define nou_symbol_match              '='
#define nou_symbol_value              '^'
#define nou_symbol_negate             '!'
#define nou_symbol_bind               '~'
#define nou_symbol_datablock          '%'   // for array-like access of graphs
#define nou_symbol_literal            '`'   // Thunk-like (or quote like?)
// Graph types
#define nou_symbol_stream             '$'   // A url and associated stream handle
#define nou_symbol_channel            '#'   // A channel providing an access type to a resource
#define nou_symbol_cut                '/'   // Used to close a channel
#define nou_symbol_out                '<'
#define nou_symbol_in                 '>'
#define nou_symbol_lambda             '\\'  // Lambda operator
// Macros affect the way tokens after the initial graph are read
#define nou_symbol_macro              '@'
#define nou_symbol_list               '.'
#define nou_symbol_extract            '-'
#define nou_symbol_emplace            '+'
#define nou_symbol_unify              '&'
#define nou_symbol_open_expression    '('
#define nou_symbol_close_expression   ')'
// #define nou_symbol_socket             '*'


/* Nouveau
**  Primitive functions
*/

nou_graph_t * nou_graph_set_functor(nou_graph_t * node, void * functor) {
  if(!node) return NULL;
  if(!node->functor) {
    node->functor = NULL;
    node->type |= 0ul << 1;
  } else {
    node->functor = functor;
    node->type |= 1ul << 1;
  }
  return node->functor;
}

nou_graph_t * nou_graph_set_head(nou_graph_t * node, nou_graph_t * head) {
  if(!node) return NULL;
  if(!node->head) {
    node->head = NULL;
    node->type |= 0ul << 2;
  } else {
    node->head = head;
    node->type |= 1ul << 2;
  }
  return node->head;
}

nou_graph_t * nou_graph_set_body(nou_graph_t * node, nou_graph_t * body) {
  if(!node) return NULL;
  if(!node->body) {
    node->body = NULL;
    node->type |= 0ul << 3;
  } else {
    node->body = body;
    node->type |= 1ul << 3;
  }
  return node->body;
}

nou_graph_t * nou_graph_create(void * functor,
                                nou_graph_t * head
                                nou_graph_t * body )
{
  nou_graph_t * graph = malloc(sizeof(nou_graph_t));
  if(!graph) return NULL;
  graph->type = 0;
  if(!nou_graph_set_functor(graph, functor)) return NULL;
  if(!nou_graph_set_head(graph, head)) return NULL;
  if(!nou_graph_set_body(graph, body)) return NULL;
  return graph;
}

nou_graph_t * nou_graph_tag(nou_graph_t * node) {
  if(!node) return NULL;
  node->type |= 1ul << 4;
  return node;
}

nou_graph_t * nou_graph_untag(nou_graph_t * node) {
  if(!node) return NULL;
  node->type |= 0ul << 4;
  return node;
}

bool nou_graph_tagged(nou_graph_t * node) {
  return node->type > 6;
}

bool nou_graph_destroy(nou_graph_t * graph) {
  bool destroyed = false;
  if(!graph) return false;
  if(graph->type > 0) {
    destroyed = nou_graph_destroy(graph->head);
    destroyed = destroyed && nou_graph_destroy(graph->body);
    destroyed = destroyed && nou_graph_destroy(graph->functor);
  }
  free(graph);
  graph = NULL;
  return destroyed;
}


bool nou_graph_match( nou_graph_t * bindings,
                      nou_graph_t * pattern,
                      nou_graph_t * term)
{
  return ( pattern->functor == term->functor
          && (pattern->head == pattern->head)
          && (pattern->body == term->body) )
        ? true : false;
}


/* Nouveau
*   
*/




/* nou_define is a unique-entry (char*) set with nodes shaped:
** internal: ~ [ symbol term ] [ symbol term ] ... */
nou_graph_t * nou_bind( nou_graph_t * super_bindings,
                            nou_graph_t * bindings,
                            const char * symbol,
                            nou_graph_t * term,
                            bool rebind_term)
{
  nou_graph_t * current_binding = bindings;
  nou_graph_t * existing_term = NULL;
  if(!bindings)
    return nou_graph_create(strndup(nou_symbol_bind, NOUVEAU_SYMBOL_SIZE), strndup(symbol, NOUVEAU_SYMBOL_SIZE), term);
  while(current_binding) {
    if(strncmp(symbol, (char*) current_binding->head, NOUVEAU_SYMBOL_SIZE)) {
      existing_term = current_binding->body;
      if(rebind_term)
        *existing_term = term;
      break;
    }
    current_binding = current_binding->functor;
  }
  if(!existing_term) {
    if(super_bindings) {
      return nou_bind(NULL, bindings, symbol, term, rebind_term);
    } else {
      current_binding = nou_graph_create( bindings, nou_atom(bindings, symbol), term);
      *bindings = current_binding;
    }
    return term;
  }
  return existing_term;
}

nou_graph_t * nou_atom( nou_graph_t * bindings,
                            const char * symbol)
{
  return nou_graph_create(NULL, nou_bind(bindings, symbol, NULL), NULL, NULL, false);
}

#define nou_true(b)       (nou_atom(b,":true"))
#define nou_false         \(0)

// block [size data]
nou_graph_t * nou_block(size_t size, void * data) {
  void * block = malloc(size);
  if(!block) return NULL;
  block = data;
  return nou_graph_create(nou_symbol_block, size, block);
}

nou_graph_t * nou_list( nou_graph_t * bindings,
                            nou_graph_t * head,
                            nou_graph_t * tail)
{
  return nou_graph_create(nou_atom(bindings, nou_symbol_list), head, tail);
}

nou_graph_t * nou_scope(nou_graph_t * super_scope,
                        nou_graph_t * bindings,
                        nou_graph_t * execution)
{
  return nou_graph_create(nou_atom(bindings, ))
}

/* Nouveau
**  context and scoping
*/
typedef struct Scope {
  struct Scope * super_scope;
  nou_graph_t * bindings;
  bool logical;
  bool marked;
} scope_t;


/* Nouveau
**  built-ins
*/

// graph [functor [head body]] => constructs a graph homoiconically, this is like a lisp-quote
nou_graph_t * nou_fn_graph( nou_graph_t * graph) {
  nou_graph_t * bindings = graph->functor;
  nou_graph_t * symbol = graph->head;
}

// bind [symbol term] => binds term to symbol in the scope bindings
nou_graph_t * nou_fn_bind( nou_graph_t * graph) {
  nou_graph_t * bindings = graph->functor;
  nou_graph_t * symbol = graph->head;
  nou_graph_t * term = graph->body;
  return nou_bind(NULL, bindings, symbol, term);
}

// match [x y] => recursively matches node x with y then yields (:true) or (0)
nou_graph_t * nou_fn_match( nou_graph_t * node) {
  return nou_match(node->functor, node->head, node->body);
}

/* value [:functor body] | value [:head body] | value [:body body] =>
    returns the symbol from a specific unit of a graph
*/
nou_graph_t * nou_fn_value(nou_graph_t * node) {
  nou_graph_t * value_symbol = nou_symbol(node->functor, node->head);
  nou_graph_t * graph = node->body;
  if(nou_symbol(node->functor, ":functor") == value_symbol) {
    return graph->functor;
  } else if(nou_symbol(node->functor, ":head") == value_symbol) {
    return graph->head;
  } else if(nou_symbol(node->functor, ":body") == value_symbol) {
    return graph->body;
  }
}

// resource [ url stream ] => $ [ url stream ]
nou_graph_t * nou_fn_resource(nou_graph_t * node) {
  nou_graph_t * url_symbol = nou_symbol(node->functor, node->head);
  nou_graph_t * stream_symbol = NULL;
  if(nou_symbol(node->functor, ":std") == url_symbol) {
    stream_symbol = stdin;
  } else if(nou_symbol(node->functor, "std")
    
  } else { // when stream is symbol
    stream = nou_symbol(node->functor, node->body);
  }
  return nou_graph( nou_symbol(node->functor, nou_symbol_resource), url_symbol, stream_symbol);
}


// channel [ mode $[url] ] => # [ mode $ [ url stream ]]
nou_graph_t * nou_fn_channel(nou_graph_t * node) {
  nou_graph_t * mode_symbol = nou_symbol(node->functor, node->head);
  nou_graph_t * resource_symbol = nou_symbol(node->functor, node->body);
  FILE * stream = stdin;
  
  if(nou_symbol(node->functor, ":std") == mode_symbol) {

  }

  switch(mode_symbol) {
    case ":in":
      stream = stdin;
    break;
    case ":out":
      stream = stdout;
    break;
    // case ":stdarg":
    //   // return nou_argument();
    // break;
    case ":stderr":
      stream = stderr;
    break;
    //@TODO: Validate mode for all possible a, b, w, r, +, x combinations
    default: // Must be a file
      char * mode = mode_symbol;
      if(mode[0] == '':' && mode[1] != '\0')
        *mode++;
      stream = fopen(resource_symbol->head, mode);
      resource_symbol->body = stream;
    break;
  }
  return nou_graph( nou_symbol(node->functor, nou_symbol_channel), mode_symbol, nou_fn_resource(resource_symbol, stream));
}

// nou_graph_t * nou_fn_open(nou_graph_t * node) {
  
// }

// nou_graph_t * nou_fn_close(nou_graph_t * node) {
  
// }

// nou_graph_t * nou_fn_read(nou_graph_t * node) {
  
// }

// nou_graph_t * nou_fn_write(nou_graph_t * node) {
  
// }

// // rotate [:left x] | rotate [:right x] | rotate [:ccw x] | rotate [:cw x]
// nou_graph_t * nou_fn_rotate(nou_graph_t * node) {
//   nou_graph_t * direction_symbol = nou_symbol(node->functor, node->head;);
//   nou_graph_t * graph = node->body;
//   void * graph_functor = graph->functor; // save (f) : f [h b]
//   if(":left" == direction_symbol || ":ccw" == direction_symbol) { // f [h b] => b [f h]
//     graph->functor = graph->body; // set f [h b] => b [h b]
//     graph->body = graph->head;    // set b [h b] => b [h h]
//     graph->head = graph_functor;  // set b [h h] => b [(f) h]
//     return graph;
//   } else if(":right" == direction_symbol || ":cw" == direction_symbol) { // f [h b] => h [b f]
//     graph->functor = graph->head; // set f [h b] => h [h b]
//     graph->head = graph->body;    // set h [h b] => h [b b]
//     graph->body = graph_functor;  // set h [b b] => h [b (f)]
//     return graph;
//   }

//   void * graph = node->body;
//   return node;
// }

// // swap [:head x] | swap [:body x]
// nou_graph_t * nou_fn_swap(nou_graph_t * node) {
//   void * graph = node->head;
//   node->head = node->body;
//   node->body = graph;
//   return node;
// }

// nou_graph_t * nou_fn_twist(nou_graph_t * node) {
//   nou_graph_t * graph = node->head;
//   node->head = node->body;
//   node->body = head_data;
//   return node;
// }

// typedef struct Nou {
//   nou_graph_t * symbols;
//   nou_graph_t * program;
//   nou_graph_t * parser;
//   int argc;
//   char argv;
// } nou_t;


















// enum Nouveau_Bytecode {
//   /* Process calculus */
//   NOU_NIL = 0x01,
//   NOU_OUTPUT,
//   NOU_INPUT,
//   NOU_PARALLEL,
//   NOU_EITHER,
//   NOU_RESTRICT,
//   NOU_REPLICATE,
//   NOU_GRAPH,
//   /* System processes */
//   NOU_FFI,
//   NOU_STD,
//   NOU_ERR,
//   NOU_FILE,
//   NOU_PATH,
//   /* Math */
//   NOU_ADD,
//   NOU_SUB,
//   NOU_MUL,
//   NOU_DIV,
//   NOU_MOD,
//   NOU_DOT,
//   NOU_CROSS,
//   NOU_FLOOR,
//   NOU_CEIL,
//   NOU_SIN,
//   NOU_COS,
//   NOU_ACOS,
//   NOU_ATAN,
//   NOU_ATAN2,
// };

// struct Nouveau_Engine {
//   uint32_t register_values[NOUVEAU_REGISTERS];
//   void * thread_data[NOUVEAU_THREADS];
//   thrd_t * thread_pool[NOUVEAU_THREADS];
//   struct Nouveau_Process master_processes[NOUVEAU_THREADS];
//   struct Nouveau_Process * processes;
//   struct Nouveau_Script * scripts;
//   size_t process_count;
//   size_t script_count;
//   int state;
// };

// void nou_make_engine(int * argc, char ** argv);
// void nou_destroy_engine(struct Nouveau_Engine * nou);

// struct Nouveau_Channel * nou_make_channel(struct Nouveau_Graph * input,
//                                           struct Nouveau_Graph * output
//                                           Nouveau_Transformation transform)


// void nou_load(struct Nouveau_Engine * nou);
// void nou_execute(struct Nouveau_Engine * nou);

// void nou_nil(struct Nouveau_Engine * nou);
// void nou_output(struct Nouveau_Engine * nou);
// void nou_input(struct Nouveau_Engine * nou);
// void nou_parallel(struct Nouveau_Engine * nou);
// void nou_restrict(struct Nouveau_Engine * nou); // create channel in process->symbols, increase symbol_count
// void nou_replicate(struct Nouveau_Engine * nou);

// void nou_read(struct NouveauEngine * nou);
// void nou_eval()
// void nou_print(struct NouveauEngine * nou);

// void nou_repl(struct Nouveau_Process * nou);















// static bool nou_token_whitespace(int * ch, char * token, int * index, bool emit_space) {
//   bool whitespace = isspace(*c);
//   while(isspace(*c))
//     *c = getchar();
//   if(emit_space) nou_token_emit(nou, ' ');
//   return whitespace;
// }

// static bool nou_token_symbol(int * c, bool emit_symbol) {
//   bool symbol = (*c != EOF && !isspace(*c) && !nou_is_symbol_block(*c) && isgraph(*c));
//   while(*c != EOF && !isspace(*c) && !nou_is_symbol_block(*c) && isgraph(*c)) {
//     if(emit_symbol)
//       nou_token_emit(nou, *c);
//     *c = getchar();
//   }
//   if(emit_symbol) nou_token_emit(nou, *c);
//   return symbol;
// }

// static bool nou_token_block(int * c) {
//   int *c = nou->scanner;
//   if (nou_is_symbol_block(*c)) {
//     nou_token_emit(nou, *c);
//     c = getchar();
//     return true;
//   } else {
//     while(*c != EOF && !isspace(*c)) {
//       nou_token_emit(nou, *c);
//       *c = getchar();
//     }
//     nou_token_whitespace(nou, true);
//     while(*c != EOF && !isspace(*c) && !nou_is_symbol_block(*c)) {
//       nou_token_emit(nou, *c);
//       *c = getchar();
//     }
//   }
//   return false;
// }

// static bool nou_token_graph(nou_t * nou) {
//   int * c = nou->scanner;
//   while(*c != EOF && !nou_is_semicolon(*c)) {
//     nou_token_emit(nou, *c);
//     c = getchar();
//   }
//   if(nou_is_semicolon(*c)) {
//     nou_token_emit(nou, *c);
//     c = getchar();
//     return true;
//   }
//   return false;
// }

// static bool nou_token_expression(nou_t * nou) {
//   int * c = nou->scanner;
//   bool atomic = false;
//   nou_token_whitespace(nou, false);
//   atomic = nou_token_symbol(nou, true);
//   nou_token_whitespace(nou, false);
//   atomic = !nou_token_block(nou);
//   if(atomic)
//     atomic = !nou_token_graph(nou);
//   return atomic;
// }

// static void nou_token(nou_t * nou) {
//   nou->symbol = 0;
//   bool atomic = nou_token_expression(nou);
//   nou->token[nou->symbol] = '\0';
// }

// static void * nou_get_object(nou_t * nou) {
//   int * c = nou->token;
//   if (*c != EOF && !isspace(*c))
//     return nou_get_graph(nou);
//   return nou_symbol(nou->token);
// }

// nou_graph_t * nou_get_graph(nou_t * nou) {
//   int * c = nou->token[0];
//   nou->symbol = 0;
//   if(*c == '[') // If there is no functor, recursively create a NULL functor graph
//     return nou_graph(NULL, nou_get_graph(nou), nou_get_graph(nou));
//   nou_token(nou); // Capture functor token
//   if(nou->token[0] == ']' || nou->token[0] == ';')
//     return NULL;


//   graph = nou_getobj(nou);
//   return nou_graph(graph, nou_get_graph(nou), nou_get_graph(nou));
// }


//   void * getobj() {
//     if (token[0] == '(') return getlist();
//     return intern(token);
//   }


//   List * getlist() {
//     List *tmp;
//     gettoken();
//     if (token[0] == ')') return 0;
//     tmp = getobj();
//     return cons(tmp, getlist());
//   }


// static void nou_readobj(nou_t * nou, const char * line) {

// }













#endif
