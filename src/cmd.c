/* edit
 * Edit commands implementation
 */

#include <stddef.h>
#include <string.h>

#include "cmd.h"
#include "line.h"

static void _shift_down(CommandStack *cmds);

/* Initializes the stack of commands */
void cmd_init(CommandStack *cmds) {
	memset(cmds->cmds, 0, sizeof(*cmds->cmds) * MAX_COMMANDS);
	cmds->length = 0;
}

/* Pushes a command into the stack */
void cmd_push(CommandStack *cmds, Command cmd) {
	if( cmds->length == MAX_COMMANDS - 1 ) {
		_shift_down(cmds);
	}

	cmds->cmds[cmds->length++] = cmd;
}

/* Pops a command from the stack */
Command *cmd_pop(CommandStack *cmds) {
	if( cmds->length == 0 ) {
		return NULL;
	}

	return &cmds->cmds[--cmds->length];
}

/* Pushes a CMD_REP_CH command to the command stack */
void cmd_rep_ch(CommandStack *cmds, size_t line, size_t idx, char ch) {
	Command cmd = {
		.type = CMD_REP_CH,
		.line = line,
		.idx = idx,
		.data.ch = ch,
	};

	cmd_push(cmds, cmd);
}

/* Pushes a CMD_ADD_CH command to the command stack */
void cmd_add_ch(CommandStack *cmds, size_t line, size_t idx, char ch) {
	Command cmd = {
		.type = CMD_ADD_CH,
		.line = line,
		.idx = idx,
		.data.ch = ch,
	};

	cmd_push(cmds, cmd);
}

/* Pushes a CMD_DEL_CH command into the command stack */
void cmd_del_ch(CommandStack *cmds, size_t line, size_t idx, char ch) {
	Command cmd = {
		.type = CMD_DEL_CH,
		.line = line,
		.idx = idx,
		.data.ch = ch,
	};

	cmd_push(cmds, cmd);
}

/* Pushes a CMD_NEW_LINE command to the command stack */
void cmd_new_line(CommandStack *cmds, size_t line, size_t idx) {
	Command cmd = {
		.type = CMD_NEW_LINE,
		.line = line,
		.idx = idx,
		.data.ch = '\0',
	};

	cmd_push(cmds, cmd);
}

/* Pushes a CMD_ADD_LINE command to the command stack */
void cmd_add_line(CommandStack *cmds, size_t line, size_t idx, Line l) {
	Command cmd = {
		.type = CMD_ADD_LINE,
		.line = line,
		.idx = idx,
		.data.line = l,
	};

	cmd_push(cmds, cmd);
}

/* Pushes a CMD_DEL_LINE command into the command stack */
void cmd_del_line(CommandStack *cmds, size_t line, size_t idx, Line l) {
	Command cmd = {
		.type = CMD_DEL_LINE,
		.line = line,
		.idx = idx,
		.data.line = l,
	};

	cmd_push(cmds, cmd);
}

/* Frees a command from memory */
void cmd_free_cmd(Command *cmd) {
	if( cmd->type == CMD_ADD_LINE || cmd->type == CMD_DEL_LINE ) {
		line_free(&cmd->data.line);
	}
}

/* Shifts a command stack down, overwriting the first command */
static void _shift_down(CommandStack *cmds) {
	cmd_free_cmd(&cmds->cmds[0]);
	for( size_t i = 1; i < cmds->length; ++i ) {
		cmds->cmds[i - 1] = cmds->cmds[i];
	}

	--cmds->length;
}
