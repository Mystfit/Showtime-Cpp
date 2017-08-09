#pragma once

#include <iostream>
#include "ZstExports.h"
#include "ZstURI.h"
#include "ZstCable.h"
#include "ZstEvent.h"

class ZstInputPlug;
class ZstPlugDataEventCallback {
public:
	ZST_EXPORT virtual ~ZstPlugDataEventCallback() { std::cout << "Destroying input callback" << std::endl; }
	ZST_EXPORT virtual void run(ZstInputPlug * e) { std::cout << "Input callback running" << std::endl; }
};

class ZstPlugEventCallback {
public:
	ZST_EXPORT virtual ~ZstPlugEventCallback() { std::cout << "Destroying plug event callback" << std::endl; }
	ZST_EXPORT virtual void run(ZstURI plug) { std::cout << "Running plug event callback" << std::endl; }
};

class ZstEntityEventCallback {
public:
	ZST_EXPORT virtual ~ZstEntityEventCallback() { std::cout << "Destroying performer event callback" << std::endl; }
	ZST_EXPORT virtual void run(ZstURI performer) { std::cout << "Running performer event callback" << std::endl; }
};

class ZstCableEventCallback {
public:
	ZST_EXPORT virtual ~ZstCableEventCallback() { std::cout << "Destroying cable event callback" << std::endl; }
	ZST_EXPORT virtual void run(ZstCable cable) { std::cout << "Running cable event callback" << std::endl; }
};

class ZstEventCallback {
public:
	ZST_EXPORT virtual ~ZstEventCallback() { std::cout << "Destroying stage event callback" << std::endl; }
	ZST_EXPORT virtual void run(ZstEvent e) { std::cout << "Running stage event callback" << std::endl; }
};


class ZstInputPlug;
class ZstComponent;
class ZstComputeCallback : public ZstPlugDataEventCallback {
public:
    ZST_EXPORT void set_target_filter(ZstComponent * component);
    ZST_EXPORT void run(ZstInputPlug * plug) override;
private:
    ZstComponent * m_component;
};
