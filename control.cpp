#include <unistd.h>
#include <vector>
#include <limits>

#include "topology.hpp"
#include "zmq_std.hpp"

using node_id_type = long long;

int main() {
	int rc;
	topology_t<node_id_type> control_node;
	std::vector< std::pair<void*, void*> > childs;

	std::string s;
	node_id_type id;
	while (std::cin >> s) {
		if (s == "create") {
			node_id_type parent_id;
			std::cin >> id >> parent_id;
			if (parent_id == -1) {
				void* new_context = NULL;
				void* new_socket = NULL;
				zmq_std::init_pair_socket(new_context, new_socket);
				rc = zmq_bind(new_socket, ("tcp://*:" + std::to_string(PORT_BASE + id)).c_str());
				assert(rc == 0);

				int fork_id = fork();
				if (fork_id == 0) {
					rc = execl(NODE_EXECUTABLE_NAME, NODE_EXECUTABLE_NAME, std::to_string(id).c_str(), NULL);
					assert(rc != -1);
					return 0;
				} else {
					bool ok = true;
					node_token_t reply_info({fail, id, id});
					ok = zmq_std::recieve_msg_wait(reply_info, new_socket);

					node_token_t* token = new node_token_t({ping, id, id});
					node_token_t reply({fail, id, id});
					ok = zmq_std::send_recieve_wait(token, reply, new_socket);
					if (ok and reply.action == success) {
						childs.push_back(std::make_pair(new_context, new_socket));
						control_node.insert(id);
						std::cout << "OK: " << reply_info.id << std::endl;
					} else {
						rc = zmq_close(new_socket);
						assert(rc == 0);
						rc = zmq_ctx_term(new_context);
						assert(rc == 0);
					}
				}
			} else if (control_node.find(parent_id) == -1) {
				std::cout << "Error: Not found" << std::endl;
			} else {
				if (control_node.find(id) != -1) {
					std::cout << "Error: Already exists" << std::endl;
				} else {
					int ind = control_node.find(parent_id);
					node_token_t* token = new node_token_t({create, parent_id, id});
					node_token_t reply({fail, id, id});
					if (zmq_std::send_recieve_wait(token, reply, childs[ind].second) and reply.action == success) {
						std::cout << "OK: " << reply.id << std::endl;
						control_node.insert(parent_id, id);
					} else {
						std::cout << "Error: Parent is unavailable" << std::endl;
					}
				}
			}
		} else if (s == "pingall") {
			bool allprocess_flag = true;
			std::vector<node_id_type> active_node;
			for (node_id_type id : control_node.getAllPointers()) {
                int ind = control_node.find(id);
                node_token_t* token = new node_token_t({ping, id, id});
				node_token_t reply({fail, id, id});
				if (zmq_std::send_recieve_wait(token, reply, childs[ind].second) and reply.action == success) {
					active_node.push_back(id);
				} else {
					allprocess_flag = false;
				}
            }
			if (allprocess_flag) {
				std::cout << "OK: -1" << std::endl;
			} else {
				std::cout << "OK: ";
				for (auto elem: active_node) {
					std::cout << elem << " ";
				}
				std::cout << std::endl;
			}
		} else if (s == "exec") {
			std::string key, value, res;
			std::cin >> id >> key >> value;
			int ind = control_node.find(id);
			if (ind != -1) {
				bool ok = true;
				int responce;
				res = key + SENTINEL + value + SENTINEL;
				std::cout << res << std::endl;
				for (size_t i = 0; i < res.size(); ++i) {
					node_token_t* token = new node_token_t({exec, res[i], id});
					node_token_t reply({fail, id, id});
					if (!zmq_std::send_recieve_wait(token, reply, childs[ind].second) or reply.action != success) {
						ok = false;
						break;
					}
					if (i == res.size() - 1) {
						responce = reply.parent_id;
					}
				}
				if (ok) {
					std::cout << "OK: " << responce << std::endl;

				} else {
					std::cout << "Error: Node is unavailable" << std::endl;
				}
			} else {
				std::cout << "Error: Not found" << std::endl;
			}
		}
	}

	for (size_t i = 0; i < childs.size(); ++i) {
		rc = zmq_close(childs[i].second);
		assert(rc == 0);
		rc = zmq_ctx_term(childs[i].first);
		assert(rc == 0);
	}
}