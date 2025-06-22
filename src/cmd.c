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

/* Pushes a REP_CH command to the command stack */
void cmd_rep_ch(CommandStack *cmds, size_t line, size_t idx, char ch) {
	Command cmd = {
		.type = CMD_REP_CH,
		.line = line,
		.idx = idx,
		.data.ch = ch,
	};

	cmd_push(cmds, cmd);
}

/* Pushes an ADD_CH command to the command stack */
void cmd_add_ch(CommandStack *cmds, size_t line, size_t idx, char ch) {
	Command cmd = {
		.type = CMD_ADD_CH,
		.line = line,
		.idx = idx,
		.data.ch = ch,
	};

	cmd_push(cmds, cmd);
}

/* Pushes a DEL_CH command into the command stack */
void cmd_del_ch(CommandStack *cmds, size_t line, size_t idx, char ch) {
	Command cmd = {
		.type = CMD_DEL_CH,
		.line = line,
		.idx = idx,
		.data.ch = ch,
	};

	cmd_push(cmds, cmd);
}

/* Pushes an ADD_LINE command to the command stack */
void cmd_add_line(CommandStack *cmds, size_t line, size_t idx, Line l) {
	Command cmd = {
		.type = CMD_ADD_LINE,
		.line = line,
		.idx = idx,
		.data.line = l,
	};

	cmd_push(cmds, cmd);
}

/* Pushes a DEL_LINE command into the command stack */
void cmd_del_line(CommandStack *cmds, size_t line, size_t idx, Line l) {
	Command cmd = {
		.type = CMD_DEL_LINE,
		.line = line,
		.idx = idx,
		.data.line = l,
	};

	cmd_push(cmds, cmd);
}

/* "Inverts" a command
 *
 * That is, an ADD_CH becomes a DEL_CH (delete instead of adding,) an ADD_LINE
 * becomes a DEL_LINE, etc.
 *
 * This is used to turn an undo action into a redo action, and vice-versa
 */
Command cmd_invert(Command *cmd) {
	Command out = *cmd;

	switch( cmd->type ) {
	case CMD_REP_CH: /* This one's the same */
		break;
	case CMD_ADD_CH:
		out.type = CMD_DEL_CH;
		++out.idx;
		break;
	case CMD_DEL_CH:
		out.type = CMD_ADD_CH;
		--out.idx;
		break;
	case CMD_ADD_LINE:
		out.type = CMD_ADD_LINE;
		line_clone(&cmd->data.line, &out.data.line, true);
		break;
	case CMD_DEL_LINE:
		out.type = CMD_DEL_LINE;
		line_clone(&cmd->data.line, &out.data.line, true);
		break;
	}

	return out;
}

/* Frees a command from memory */
void cmd_free_cmd(Command *cmd) {
	if( cmd->type == CMD_ADD_LINE || cmd->type == CMD_DEL_LINE ) {
		line_free(&cmd->data.line);
	}
}

static void _shift_down(CommandStack *cmds) {
	cmd_free_cmd(&cmds->cmds[0]);
	for( size_t i = 1; i < cmds->length; ++i ) {
		cmds->cmds[i - 1] = cmds->cmds[i];
	}

	--cmds->length;
}
