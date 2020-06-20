#pragma once

#include <showtime/ZstExports.h>
#include <showtime/entities/ZstComponent.h>
#include <showtime/entities/ZstPlug.h>
#include <memory>

#define ADDITION_FILTER_TYPE "addition"

class Adder : public showtime::ZstComponent {
public:
	ZST_PLUGIN_EXPORT Adder(const char * name);
	ZST_PLUGIN_EXPORT virtual void on_registered() override;
	ZST_PLUGIN_EXPORT virtual void compute(showtime::ZstInputPlug * plug) override;
	ZST_PLUGIN_EXPORT showtime::ZstInputPlug* augend();
	ZST_PLUGIN_EXPORT showtime::ZstInputPlug* addend();
	ZST_PLUGIN_EXPORT showtime::ZstOutputPlug* sum();

private:
	std::unique_ptr<showtime::ZstInputPlug> m_augend;
	std::unique_ptr<showtime::ZstInputPlug> m_addend;
	std::unique_ptr<showtime::ZstOutputPlug> m_sum;
};
