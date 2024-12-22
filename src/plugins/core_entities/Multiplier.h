#pragma once

#include <showtime/ZstExports.h>
#include <showtime/entities/ZstComputeComponent.h>
#include <showtime/entities/ZstPlug.h>
#include <memory>

#define MULTIPLICATION_FILTER_TYPE "multiplication"

class Multiplier : public showtime::ZstComputeComponent {
public:
	ZST_PLUGIN_EXPORT Multiplier(const char * name);
	ZST_PLUGIN_EXPORT virtual void on_registered() override;
	ZST_PLUGIN_EXPORT virtual void compute(showtime::ZstInputPlug * plug) override;
	ZST_PLUGIN_EXPORT showtime::ZstInputPlug* multiplicand();
	ZST_PLUGIN_EXPORT showtime::ZstInputPlug* multiplier();
	ZST_PLUGIN_EXPORT showtime::ZstOutputPlug* product();

private:
	std::unique_ptr<showtime::ZstInputPlug> m_multiplicand;
	std::unique_ptr<showtime::ZstInputPlug> m_multiplier;
	std::unique_ptr<showtime::ZstOutputPlug> m_product;
};
