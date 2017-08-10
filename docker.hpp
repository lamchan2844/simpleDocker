//docker.hpp
//
//create by chenlin2844@hotmail.com
//create at 2017/08/08
//

#pragma once
#include <sys/wait.h>
#include <sys/mount.h>
#include <fcntl.h>
#include <unistd.h>
#include <sched.h>

#include <cstring>
#include <string>

#include <net/if.h>
#include <arpa/inet.h>
#include "network.h"

#define STACK_SIZE (512*512)	//child proc size

namespace docker{
	typedef int proc_statu;
   	proc_statu proc_err = -1;
	proc_statu proc_exit = 0;
	proc_statu proc_wait = 1;

	typedef struct container_config{
		std::string host_name;
		std::string root_dir;
		std::string ip;
		std::string bridge_name;
		std::string bridge_ip;
	}container_config;

	class container{
		private:
			typedef int process_pid;
			char child_stack[STACK_SIZE];
			container_config config;

			char *veth1;
			char *veth2;
			void start_bash(){
				std::string bash = "/bin/bash";
				char *c_bash = new char[bash.size()+1];
				strcpy(c_bash, bash.c_str());
				char *const child_args[] = {c_bash, NULL};
				execv(child_args[0], child_args);
				delete []c_bash;
			}
			void set_hostname(){
				sethostname(this->config.host_name.c_str(), this->config.host_name.length());
			}
			void set_rootdir(){
				//chdir system call, change dir
				chdir(this->config.root_dir.c_str());

				chroot(".");
			}
			void set_procsys(){
				mount("none","/proc","proc",0,nullptr);
				mount("none","/sys","sysfs",0,nullptr);
			}
			void set_network(){
				printf("set_network\n");
				int ifindex = if_nametoindex("eth0");
				printf("ifindex = %d\n",ifindex);
				struct in_addr ipv4;
				struct in_addr bcast;
				struct in_addr gateway;

				inet_pton(AF_INET, this->config.ip.c_str(), &ipv4);
				inet_pton(AF_INET, "255.255.255.0", &bcast);
				inet_pton(AF_INET, this->config.bridge_ip.c_str(), &gateway);

				lxc_ipv4_addr_add(ifindex, &ipv4, &bcast, 16); 	//config eth0 ip address
				
				lxc_netdev_up("lo");
				lxc_netdev_up("eth0"); 		//active eth0

				lxc_ipv4_gateway_add(ifindex, &gateway);	//config gateway;

				char mac[18];
				new_hwaddr(mac);
				printf("mac address : %s\n",mac);
				setup_hw_addr(mac, "eth0");

				printf("set network successs!\n");
			}
		
		public:
			container(container_config &config){
				this->config = config;
			}
			void start(){
				printf("starting...\n");
				char veth1buf[IFNAMSIZ] = "chenlin0X";
				char veth2buf[IFNAMSIZ] = "chenlin0X";
				veth1 = lxc_mkifname(veth1buf);
				veth2 = lxc_mkifname(veth2buf);
				printf("veth1:%s\nveth2:%s\n",veth1buf,veth2buf);
				lxc_veth_create(veth1,veth2);
				setup_private_host_hw_addr(veth1);	//set veth1's mac address
				lxc_bridge_attach(config.bridge_name.c_str(),veth1); 	//add veth1 to net bridge
				lxc_netdev_up(veth1);	//active veth1

				auto setup = [](void *args) -> int{
					auto _this = reinterpret_cast<container *>(args);
					_this->set_hostname();
					_this->set_rootdir();
					_this->set_procsys();
					//configure container
					_this->set_network();
					
					_this->start_bash();
					printf("configuring\n");				
					return proc_wait;
				};

				process_pid child_pid = clone(setup, child_stack+STACK_SIZE, //move to stack bottom;
						CLONE_NEWUTS|
						CLONE_NEWNS|
						CLONE_NEWPID|
						CLONE_NEWNET|
						SIGCHLD,	//child proc exit will send sig to parent proc;
						this);
				lxc_netdev_move_by_name(veth2,child_pid,"eth0");
				waitpid(child_pid, nullptr, 0);//waiting child proc exit
			}
			~container(){
				lxc_netdev_delete_by_name(veth1);
				lxc_netdev_delete_by_name(veth2);

			}

	};
}
