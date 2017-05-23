/*
 * ServerCommand.cpp
 *
 *  Created on: May 15, 2017
 *      Author: nico
 */
#include <stdio.h>
#include <stdlib.h>
#include <unistd.h>
#include <sys/types.h>
#include <sys/wait.h>
#include "ServerCommand.h"

ServerCommand::ServerCommand() {
	memset(&command_list, 0, sizeof (command_list));
	command_number = 0;
}

ServerCommand::~ServerCommand() {
	for (uint32_t i; i < SERVER_COMMAND_MAX_NUMBER; i++) {
		if (command_list[i].command != 0)
			free(command_list[i].command);
	}
}



bool ServerCommand::commandRegister(const char *cmd, server_command_func_t *command_func) {
	uint32_t i;

	if (cmd == 0 || command_func == 0) {
		std::cout << "Invalid parameter" << endl;
		return false;
	}
	if (command_number == SERVER_COMMAND_MAX_NUMBER) {
		std::cout << "Max number of command registered. Aborting" << endl;
		return false;
	}
	for (i = 0; i < SERVER_COMMAND_MAX_NUMBER; i++) {
		if (command_list[i].command != 0) {
			if (!strcmp(command_list[i].command, cmd)) {
				std::cout << "Command " << cmd << " already registered ! Aborting" << endl;
				return false;
			}
		}
	}
	// Look for a free slot
	for (i = 0; i < SERVER_COMMAND_MAX_NUMBER; i++) {
		if (command_list[i].command == 0)
			break;
	}
	// Should not happen here, since we checked command_number above.
	if (i == SERVER_COMMAND_MAX_NUMBER) {
		std::cout << "Max number of command registered. Aborting" << endl;
		return false;
	}
	command_list[i].command = strdup(cmd);
	command_list[i].func    = command_func;
	command_number++;
	return true;
}

bool ServerCommand::commandUnregister(const char *cmd) {
	uint32_t i;

	if (cmd == 0) {
		std::cout << "cmd is null" << endl;
		return false;
	}
	if (command_number == 0) {
		std::cout << "No command registered so far" << endl;
		return false;
	}
	for (i = 0; i < SERVER_COMMAND_MAX_NUMBER; i++) {
		if (command_list[i].command == 0)
			continue;
		if (!strcmp(cmd, command_list[i].command)) {
			free(command_list[i].command);
			memset(command_list + i, 0, sizeof(server_command_t));
			command_number--;
			return true;
		}
	}
	// COmmand not found...
	std::cout << "Command " << cmd << " was not found in the list" << endl;
	return false;
}

bool ServerCommand::commandProcess(LonetSIM808 *lonet_ptr, const char *cmd, const char *sms_to) {
	uint32_t i, sms_id;
	server_answer_t answer;

	if (cmd == 0 || lonet_ptr == 0 || sms_to == 0) {
		std::cout << "Invalid argument" << endl;
		return false;
	}

	if (!lonet_ptr->isInitialized()) {
		std::cout << "Error: lonet object is not initialized" << endl;
		return false;
	}

	for (i = 0; i < SERVER_COMMAND_MAX_NUMBER; i++) {
		if (command_list[i].command != 0) {
			if (!strcmp(command_list[i].command, cmd)) {
				if (command_list[i].func != 0) {
					if (command_list[i].func(lonet_ptr, &answer)) {
						// Sleep a bit to let TE recover from this done in func pointer
						usleep(500*1000);
						if (!lonet_ptr->smsSend(sms_to, answer.content, &sms_id)) {
							std::cout << "Could not send answer to" << cmd << " status" << endl;
							return false;
						} else {
							std::cout << "Successfully sent answer to " << cmd << " with sms id " << to_string(sms_id) << " to " << sms_to << endl;
							std::cout << "Answer content was '" << answer.content << "'" << endl;
							return true;
						}
					} else {
						std::cout << "Function registered as " << cmd << " failed..." << endl;
						return false;
					}
				} else {
					std::cout << "Command " << cmd << " was registered without callback" << endl;
					return false;
				}
			}
		}
	}
	uint32_t offset = 0;
	std::cout << "Could not find command " << cmd << endl;
	offset = snprintf(answer.content, sizeof(answer.content), "Invalid command ");
	bool first = true;
	for (i = 0; i < SERVER_COMMAND_MAX_NUMBER; i++) {
		if (command_list[i].command != 0) {
			if (offset >= sizeof(answer.content))
				break;
			if (first) {
				offset += snprintf(answer.content + offset, sizeof(answer.content) - offset, "[%s", command_list[i].command);
				first = false;
			} else {
				offset += snprintf(answer.content + offset, sizeof(answer.content) - offset, "|%s", command_list[i].command);
			}
		}
	}
	snprintf(answer.content + offset, sizeof(answer.content) - offset, "]");
	lonet_ptr->smsSend(sms_to, answer.content, &sms_id);
	return true;
}

bool ServerCommand::commandProcessFromSms(LonetSIM808 *lonet_ptr, Sms *sms) {
	if (sms == 0|| !lonet_ptr) {
		std::cout << "Invalid parameter (null)" << endl;
		return false;
	}

	if (!lonet_ptr->isInitialized()) {
		std::cout << "Error: lonet object is not initialized" << endl;
		return false;
	}

	if (!commandProcess(lonet_ptr, sms->getText(), sms->getFrom())) {
		std::cout << "Error: unable to process command from sms" << endl;
		return false;
	}

	lonet_ptr->getSerial().flush(100);
	usleep(1000*1000);
	lonet_ptr->smsDelete(sms->getIndex());
	return true;
}

bool ServerCommand::commandPing(LonetSIM808 *lonet, server_answer_t *answer) {

	if (!lonet || !answer) {
		std::cout << "Invalid parameter" << endl;
		return false;
	}

	snprintf(answer->content, sizeof (answer->content), "Ready. Serial number: %s. Network: %s",
			lonet->getSerialNumber(), lonet->getOperator().data());
	answer->length = strlen(answer->content);
	std::cout << __func__ << "(). Sending '" << answer->content << "'" << endl;

	return true;
}

bool ServerCommand::commandBattery(LonetSIM808 *lonet, server_answer_t *answer) {

	LonetSIM808::BatteryInfo_t info;

	if (!lonet || !answer) {
		std::cout << "Invalid parameter" << endl;
		return false;
	}

	if (lonet->batteryInfoGet(true, &info)) {
		snprintf(answer->content, sizeof (answer->content), "Battery info (%s): %d/100 (%dmV)",
				info.charge_status == LonetSIM808::BATTERY_CHARGING ? "charging" :
						(info.charge_status == LonetSIM808::BATTERY_NOT_CHARGING ? "discharging" :
								(info.charge_status == LonetSIM808::BATTERY_FINISHED_CHARGING ? "fully charged" : "unknown")
						),
				info.capacity, info.voltage);
		answer->length = strlen(answer->content);
		std::cout << __func__ << "(). Sending '" << answer->content << "'" << endl;
		return true;
	} else {
		std::cout << "Unable to get battery info..." << endl;
		return false;
	}
}

bool ServerCommand::commandLocation(LonetSIM808 *lonet, server_answer_t *answer) {

	if (!lonet || !answer) {
		std::cout << "Invalid parameter" << endl;
		return false;
	}

	snprintf(answer->content, sizeof (answer->content), "No location so far");
	answer->length = strlen(answer->content);
	std::cout << __func__ << "(). Sending '" << answer->content << "'" << endl;
	return true;
}


bool ServerCommand::commandTimelapseStatus(LonetSIM808 *lonet, server_answer_t *answer) {

	if (!lonet || !answer) {
		std::cout << "Invalid parameter" << endl;
		return false;
	}

	int link[2];
	pid_t pid;
	char out[160];

	if (pipe(link) < 0) {
		std::cout << "Could not create pipe" << endl;
		return false;
	}

	if ((pid = fork()) == -1) {
		std::cout << "Could not fork" << endl;
		return false;
	}

	if (pid == 0) {
		dup2 (link[1], STDOUT_FILENO);
		close(link[0]);
		close(link[1]);
		execl("/usr/bin/python", "python", "/home/pi/fast_worker/lib/tl_monitor.py", (char *)0);
		std::cout << "Could not fork" << endl;
		exit(-1);
	} else {
		close(link[1]);
		int nbytes = read(link[0], answer->content, sizeof(answer->content));
		answer->content[nbytes] = 0;
		waitpid(pid, NULL, 0);
	  }
	answer->length = strlen(answer->content);
	std::cout << __func__ << "(). Sending '" << answer->content << "'" << endl;
	return true;
}
