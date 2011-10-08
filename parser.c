typedef struct cmd_pipeline_item {
    /* Item is:
     * this item "value":
     ** vector of arguments or pointer to list;
     * pointer to next item. */
    char **argv;
    struct cmd_list *cmd_lst;
    struct cmd_pipeline_item *next;
} cmd_pipeline_item;

typedef struct cmd_pipeline {
    char *input;
    char *output;
    unsigned int append:1;
    struct cmd_pipeline_item *first_item;
} cmd_pipeline;

typedef enum type_of_relation {
    REL_OR,    /* '||' */
    REL_AND,   /* '&&' */
    REL_BOTH   /* ';'  */
} type_of_relation;

typedef struct cmd_list_item {
    /* Item is:
     ** pointer to pipeline or list, this item "value";
     * relation with and pointer to next item. */
    struct cmd_pipeline *cmd_pl;
    struct cmd_list *cmd_lst;
    type_of_relation rel;
    struct cmd_list_item *next;
} cmd_list_item;

typedef struct cmd_list {
    unsigned int foreground:1;
    struct cmd_list_item *first_item;
}

typedef struct parser_info {
    lexer_info *linfo;
    lexeme *cur_lex;
} parser_info;

void init_parser (parser_info *pinfo)
{
    /* Пусть это делает кто-то другой, в main ()
    pinfo->linfo = (lexer_info *) malloc (sizeof (lexer_info)); */
    init_lexer (pinfo->linfo);
    pinfo->cur_lex = get_lex (pinfo->linfo);
}

void parser_get_lex (parser_info *pinfo)
{
    pinfo->cur_lex = get_lex (pinfo->linfo);
}

*cmd_pipeline_item parse_cmd_pipeline_item (parser_info *pinfo)
{
}

*cmd_pipeline parse_cmd_pipeline (parser_info *pinfo)
{
    
    switch (pinfo->cur_lex_type) {
    case 
    }
}

*cmd_list_item parse_cmd_list_item (parser_info *pinfo)
{
    parser_get_lex (pinfo->linfo);
    switch (pinfo->cur_lex->type) {
    case LEX_INPUT:         /* '<'  */
    case LEX_OUTPUT:        /* '>'  */
    case LEX_APPEND:        /* '>>' */
    case LEX_PIPE:          /* '|'  */
    case LEX_OR:            /* '||' */
    case LEX_BACKGROUND:    /* '&'  */
    case LEX_AND:           /* '&&' */
    case LEX_SEMICOLON:     /* ';'  */
    case LEX_BRACKET_CLOSE: /* ')'  */
        /* Error */
        break;
    case LEX_BRACKET_OPEN:  /* '('  */
        break;
    case LEX_REVERSE:       /* '`'  */
        break;
    case LEX_WORD:     /* all different */
        /* todo: make pipeline */
        break;
    case LEX_EOLINE:        /* '\n' */
        break;
    case LEX_EOFILE:         /* EOF  */
        break;
    }
}

*cmd_list parse_cmd_list (parser_info *pinfo)
{
    /* TODO:
    malloc cmd_list;
    cmd_list->first_item = parse_cmd_list_item (pinfo);
    cmd_list->foregroung = 1;
    switch (pinfo->cur_lex->type) {
    case LEX_BACKGROUND:
        cmd_list->foreground = 0;
        parse_get_lex (pinfo);
        break;
    case LEX_SEMICOLON:
        parse_get_lex (pinfo);
        break;
    case LEX_BRACKET_CLOSE:
        * TODO: We begin with open bracket? *
        parse_get_lex (pinfo);
        break;
    default:
        * No actions *
        break;
    }
    */
}
