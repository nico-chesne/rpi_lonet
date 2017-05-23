/*
 * ServerCommand.h
 *
 *  Created on: May 15, 2017
 *      Author: nico
 */

#ifndef SERVERCOMMAND_H_
#define SERVERCOMMAND_H_

#include <LonetSIM808.h>

#define ANSWER_MAX_LENGTH 160
#define SERVER_COMMAND_MAX_NUMBER 128


class ServerCommand {
public :
	struct server_answer_t {
		char content[ANSWER_MAX_LENGTH];
		uint32_t length;
	};
	typedef bool (server_command_func_t)(LonetSIM808 *lonet, server_answer_t *answer);

public:
	ServerCommand();
	virtual ~ServerCommand();

	bool commandRegister(const char *cmd, server_command_func_t *command_func);
	bool commandUnregister(const char *cmd);
	bool commandProcess(LonetSIM808 *lonet_ptr, const char *cmd, const char *sms_to);
	bool commandProcessFromSms(LonetSIM808 *lonet, Sms *sms);

public:
	static bool commandPing(LonetSIM808 *lonet, server_answer_t *answer);
	static bool commandBattery(LonetSIM808 *lonet, server_answer_t *answer);
	static bool commandLocation(LonetSIM808 *lonet, server_answer_t *answer);
	static bool commandTimelapseStatus(LonetSIM808 *lonet, server_answer_t *answer);


protected:
	struct server_command_t {
		char *command;
		server_command_func_t *func;
	};
	server_command_t command_list[SERVER_COMMAND_MAX_NUMBER];
	uint32_t command_number;
};

#endif /* SERVERCOMMAND_H_ */
