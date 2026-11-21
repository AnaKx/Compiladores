// ============================================================================
// BabyDuck.g4
// Fases 1 y 2: análisis léxico (scanner) y sintáctico (parser)
//
//   texto ─► SCANNER ─► tokens ─► PARSER ─► árbol ─► semántica ─► cuádruplos
//
// Qué hace:
//   Define el lenguaje BabyDuck:
//   - abajo (SCANNER): qué palabras existen (tokens) y cómo se escriben;
//   - arriba (PARSER): en qué orden pueden ir esas palabras (reglas gramaticales).
//
// Papel en el proceso:
//   No se compila directamente: ANTLR lo lee y genera en generated/ el scanner
//   (BabyDuckLexer), el parser (BabyDuckParser) y el listener base
//   (BabyDuckBaseListener). Cada regla de aquí se vuelve un método enterRegla /
//   exitRegla, que es donde semantica.hpp y generador.hpp ponen sus puntos neurálgicos.
//   Recibe: el texto del programa.  Entrega: tokens y el árbol sintáctico.
//   Si cambias este archivo, `make` vuelve a correr ANTLR.
// ============================================================================

grammar BabyDuck;

// PARSER

programa
    : PROGRAM ID SEMI varsList funcList MAIN body END EOF
    ;

varsList
    : vars varsList
    | /* ε */
    ;

vars
    : VAR idlist COLON type SEMI
    ;

idlist
    : ID idlistElem
    ;

idlistElem
    : COMMA ID idlistElem
    | /* ε */
    ;

type
    : INT
    | FLOAT
    ;

funcList
    : funcs funcList
    | /* ε */
    ;

// Una funcion puede ser void o regresar un valor (int/float)
funcs
    : (VOID | type) ID LPAREN params RPAREN LBRACKET varsList body RBRACKET SEMI
    ;

params
    : ID COLON type paramsTail
    | /* ε */
    ;

paramsTail
    : COMMA ID COLON type paramsTail
    | /* ε */
    ;

body
    : LBRACE statementList RBRACE
    ;

statementList
    : statement statementList
    | /* ε */
    ;

statement
    : retorno
    | assign
    | condition
    | cycle
    | f_call
    | print
    | read
    | LBRACKET statementList RBRACKET
    ;

retorno
    : RETURN expresion SEMI
    ;

assign
    : ID ASSIGN expresion SEMI
    ;

expresion
    : exp expRelTail
    ;

expRelTail
    : GT exp
    | LT exp
    | /* ε */
    ;

exp
    : termino expArTail
    ;

expArTail
    : PLUS termino expArTail
    | MINUS termino expArTail
    | /* ε */
    ;

termino
    : factor termTail
    ;

termTail
    : TIMES factor termTail
    | DIVIDE factor termTail
    | /* ε */
    ;

factor
    : LPAREN expresion RPAREN
    | llamada
    | signOpt ID
    | signOpt cte
    ;

signOpt
    : PLUS
    | MINUS
    | /* ε */
    ;

cte
    : CTE_INT
    | CTE_FLOAT
    ;

condition
    : IF LPAREN expresion RPAREN body elseopt SEMI
    ;

elseopt
    : ELSE body
    | /* ε */
    ;

cycle
    : WHILE LPAREN expresion RPAREN DO body SEMI
    ;

f_call
    : llamada SEMI
    ;

llamada
    : ID LPAREN arglist RPAREN
    ;

arglist
    : expresion arglistTail
    | /* ε */
    ;

arglistTail
    : COMMA expresion arglistTail
    | /* ε */
    ;

print
    : PRINT LPAREN printList RPAREN SEMI
    ;

printList
    : printItem printListTail
    ;

printListTail
    : COMMA printItem printListTail
    | /* ε */
    ;

printItem
    : expresion
    | CTE_STRING
    ;

read
    : READ LPAREN ID RPAREN SEMI
    ;

// SCANNER
// Las palabras reservadas van antes que ID

VAR : 'var' ;
PROGRAM : 'program' ;
MAIN : 'main' ;
END : 'end' ;
VOID : 'void' ;
INT : 'int' ;
FLOAT : 'float' ;
IF : 'if' ;
ELSE : 'else' ;
WHILE : 'while' ;
DO : 'do' ;
PRINT : 'print' ;
READ : 'read' ;
RETURN : 'return' ;

LPAREN : '(' ;
RPAREN : ')' ;
LBRACE : '{' ;
RBRACE : '}' ;
LBRACKET : '[' ;
RBRACKET : ']' ;
COMMA : ',' ;
SEMI : ';' ;
COLON : ':' ;

ASSIGN : '=' ;
PLUS : '+' ;
MINUS : '-' ;
TIMES : '*' ;
DIVIDE : '/' ;
GT : '>' ;
LT : '<' ;

ID : [a-zA-Z] [a-zA-Z0-9_]* ;
CTE_FLOAT : [0-9]+ '.' [0-9]+ ;
CTE_INT : [0-9]+ ;
CTE_STRING : '"' ~["\n]* '"' ;

// se ignoran espacios, tabuladores y saltos de linea
WS : [ \t\r\n]+ -> skip ;
