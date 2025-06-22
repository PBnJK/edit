#ifndef GUARD_EDIT_CMD_H_
#define GUARD_EDIT_CMD_H_

#include <stddef.h>

#include "line.h"

#define MAX_COMMANDS (64)

typedef enum _CommandType {
	CMD_REP_CH, /* Replaces a character */
	CMD_ADD_CH, /* Adds a character */
	CMD_DEL_CH, /* Deletes a character */
	CMD_NEW_LINE, /* Adds a line break */
	CMD_ADD_LINE, /* Adds a line */
	CMD_DEL_LINE, /* Deletes a line */
} CommandType;

/* Structure representing a command
 * This is used for the undo/redo system
 */
typedef struct _Command {
	CommandType type;
	size_t line;
	size_t idx;
	size_t length;
	union {
		char ch;
		Line line;
	} data;
} Command;

/* Structure representing a stack of Commands */
typedef struct _CommandStack {
	Command cmds[MAX_COMMANDS];
	size_t length;
} CommandStack;

void cmd_init(CommandStack *cmds);

void cmd_push(CommandStack *cmds, Command cmd);
Command *cmd_pop(CommandStack *cmds);

void cmd_rep_ch(CommandStack *cmds, size_t line, size_t idx, char ch);
void cmd_add_ch(CommandStack *cmds, size_t line, size_t idx, char ch);
void cmd_del_ch(CommandStack *cmds, size_t line, size_t idx, char ch);

void cmd_new_line(CommandStack *cmds, size_t line, size_t idx);
void cmd_add_line(CommandStack *cmds, size_t line, size_t idx, Line l);
void cmd_del_line(CommandStack *cmds, size_t line, size_t idx, Line l);

void cmd_free_cmd(Command *cmd);

#endif // !GUARD_EDIT_CMD_H_
