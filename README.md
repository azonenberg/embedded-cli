# Introduction

This is a simple C++ library providing a framework for a command-line interface on an embedded device.

It supports arbitrary byte-oriented transports and is intended to work over raw UARTs, SSH connections,
and more.

This library performs no dynamic memory allocation, and does not call any C/C++ library functions which
may trigger a dynamic allocation.
