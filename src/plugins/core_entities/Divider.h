#pragma once

#include <showtime/ZstExports.h>
#include <showtime/entities/ZstComputeComponent.h>
#include <showtime/entities/ZstPlug.h>
#include <memory>

#define DIVISION_FILTER_TYPE "division"

class Divider : public showtime::ZstComputeComponent {
public:
	ZST_PLUGIN_EXPORT Divider(const char * name);
	ZST_PLUGIN_EXPORT virtual void on_registered() override;
	ZST_PLUGIN_EXPORT virtual void compute(showtime::ZstInputPlug * plug) override;
	ZST_PLUGIN_EXPORT showtime::ZstInputPlug* dividend();
	ZST_PLUGIN_EXPORT showtime::ZstInputPlug* divisor();
	ZST_PLUGIN_EXPORT showtime::ZstOutputPlug* quotient();

private:
	std::unique_ptr<showtime::ZstInputPlug> m_dividend;
	std::unique_ptr<showtime::ZstInputPlug> m_divisor;
	std::unique_ptr<showtime::ZstOutputPlug> m_quotient;
};
