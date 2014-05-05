socketCS
===================
	UCLA CS 118 Lab 1
	Professor Mario Gerla
	Spring 2014

	Alan Kha		904030522	akhahaha@gmail.com
	Rukan Shao		504355612	rukan.shao@gmail.com
-------------------------------------------------------------------------------
Summary
---------------
A simple concurrent, connection-oriented client-server using BSD sockets.

Features
---------------
 - Supports GET requests.
 - Supports subdirectories.
 - Renders HTML, GIF, and JPG files in browser.

Installation
---------------
1. Run "make" to compile client.c and serverFork.c.
2. Start the server with "./serverFork ####" where #### is a port number.
3. Access a file by entering "http://127.0.0.1/filename" into a browser.
