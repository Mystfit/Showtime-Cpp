#pragma once

#include <showtime/ZstExports.h>
#include <showtime/entities/ZstComputeComponent.h>
#include <showtime/entities/ZstPlug.h>
#include <memory>

#define SUBTRACTION_FILTER_TYPE "subtraction"

class Subtractor : public showtime::ZstComputeComponent {
public:
	ZST_PLUGIN_EXPORT Subtractor(const char * name);
	ZST_PLUGIN_EXPORT virtual void on_registered() override;
	ZST_PLUGIN_EXPORT virtual void compute(showtime::ZstInputPlug * plug) override;
	ZST_PLUGIN_EXPORT showtime::ZstInputPlug* minuend();
	ZST_PLUGIN_EXPORT showtime::ZstInputPlug* subtrahend();
	ZST_PLUGIN_EXPORT showtime::ZstOutputPlug* difference();

private:
	std::unique_ptr<showtime::ZstInputPlug> m_minuend;
	std::unique_ptr<showtime::ZstInputPlug> m_subtrahend;
	std::unique_ptr<showtime::ZstOutputPlug> m_difference;
};
