/*
 * main.cpp
 * create by chenlin2844@hotmail.com
 * 
 */

#include <iostream>
#include "docker.hpp"
int main()
{
	std::cout<<"start docker...\n";	
	docker::container_config config;
	config.host_name = "king";
	config.root_dir = "../img/";
	config.ip = "192.168.0.100";
	config.bridge_name = "docker0";
	config.bridge_ip = "192.168.0.1";
	docker::container container(config);
	container.start();
	std::cout<<"stop docker...\n";
	return 0;
}
