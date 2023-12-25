#include <list>
#include <iostream>
#include <pthread.h>
#include <map>
#include <tuple>
#include <unistd.h>

#include "zmq_std.hpp"

const std::string SENTINEL_STR = "$";

long long node_id;

int main(int argc, char** argv) {
	int rc;
	assert(argc == 2);
	node_id = std::stoll(std::string(argv[1]));

	void* node_parent_context = zmq_ctx_new();
	void* node_parent_socket = zmq_socket(node_parent_context, ZMQ_PAIR);
	rc = zmq_connect(node_parent_socket, ("tcp://localhost:" + std::to_string(PORT_BASE + node_id)).c_str());
	assert(rc == 0);

	long long child_id = -1;
	void* node_context = NULL;
	void* node_socket = NULL;

	std::string value, key;
	bool flag_sentinel = true;

	node_token_t* info_token = new node_token_t({info, getpid(), getpid()});
	zmq_std::send_msg_dontwait(info_token, node_parent_socket);

	std::map<std::string, int> node_map; 
	bool has_child = false;
	bool awake = true;
	bool calc = true;
	while (awake) {
		node_token_t token;
		zmq_std::recieve_msg(token, node_parent_socket);

		node_token_t* reply = new node_token_t({fail, node_id, node_id});

		if (token.action == create) {
			if (token.parent_id == node_id) {
				if (has_child) {
					rc = zmq_close(node_socket);
					assert(rc == 0);
					rc = zmq_ctx_term(node_context);
					assert(rc == 0);
				}
				zmq_std::init_pair_socket(node_context, node_socket);
				rc = zmq_bind(node_socket, ("tcp://*:" + std::to_string(PORT_BASE + token.id)).c_str());
				assert(rc == 0);

				int fork_id = fork();
				if (fork_id == 0) {
					rc = execl(NODE_EXECUTABLE_NAME, NODE_EXECUTABLE_NAME, std::to_string(token.id).c_str(), NULL);
					assert(rc != -1);
					return 0;
				} else {
					bool ok = true;
					node_token_t reply_info({fail, token.id, token.id});
					ok = zmq_std::recieve_msg_wait(reply_info, node_socket);
					if (reply_info.action != fail) {
						reply->id = reply_info.id;
						reply->parent_id = reply_info.parent_id;
					}
					if (has_child) {
						node_token_t* token_bind = new node_token_t({bind, token.id, child_id});
						node_token_t reply_bind({fail, token.id, token.id});
						ok = zmq_std::send_recieve_wait(token_bind, reply_bind, node_socket);
						ok = ok and (reply_bind.action == success);
					}
					if (ok) {
						/* We should check if child has connected to this node */
						node_token_t* token_ping = new node_token_t({ping, token.id, token.id});
						node_token_t reply_ping({fail, token.id, token.id});
						ok = zmq_std::send_recieve_wait(token_ping, reply_ping, node_socket);
						ok = ok and (reply_ping.action == success);
						if (ok) {
							reply->action = success;
							child_id = token.id;
							has_child = true;
						} else {
							rc = zmq_close(node_socket);
							assert(rc == 0);
							rc = zmq_ctx_term(node_context);
							assert(rc == 0);
						}
					}
				}
			} else if (has_child) {
				node_token_t* token_down = new node_token_t(token);
				node_token_t reply_down(token);
				reply_down.action = fail;
				if (zmq_std::send_recieve_wait(token_down, reply_down, node_socket) and reply_down.action == success) {
					*reply = reply_down;
				}
			}
		} else if (token.action == ping) {
			if (token.id == node_id) {
				reply->action = success;
			} else if (has_child) {
				node_token_t* token_down = new node_token_t(token);
				node_token_t reply_down(token);
				reply_down.action = fail;
				if (zmq_std::send_recieve_wait(token_down, reply_down, node_socket) and reply_down.action == success) {
					*reply = reply_down;
				}
			}
		} else if (token.action == exec) {
			if (token.id == node_id) {
				bool reply_flag = false;
				char c = token.parent_id;
				if (c == SENTINEL) {
					std::cout << "Here";
					if (flag_sentinel) {
						std::swap(key, value);
					} else {
						std::swap(key, value);
						if (value == "get") {
							auto it = node_map.find(key);
							if (it != node_map.end()) {
								reply->parent_id = node_map[key];	
							} else {
								reply_flag = true;
							}
						} else {
							node_map[key] = std::stoi(value);
						}
						key.clear();
						value.clear();
					}
					flag_sentinel = flag_sentinel ^ 1;
				} else {
					key = key + c;
				}
				reply->action = success;
				if (reply_flag) {
					reply->action = notfound;
				}
			} else if (has_child) {
				node_token_t* token_down = new node_token_t(token);
				node_token_t reply_down(token);
				reply_down.action = fail;
				if (zmq_std::send_recieve_wait(token_down, reply_down, node_socket) and reply_down.action == success) {
					*reply = reply_down;
				}
			}
		}
		zmq_std::send_msg_dontwait(reply, node_parent_socket);
	}
	if (has_child) {
		rc = zmq_close(node_socket);
		assert(rc == 0);
		rc = zmq_ctx_term(node_context);
		assert(rc == 0);
	}
	rc = zmq_close(node_parent_socket);
	assert(rc == 0);
	rc = zmq_ctx_term(node_parent_context);
	assert(rc == 0);
}