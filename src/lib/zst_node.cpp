#pragma once
 
#include "zst_node.h" 

Showtime::ZstNode::ZstNode()
{
}

DLL_EXPORT Showtime::ZstNode* Showtime::ZstNode::createNode()
{
	return new ZstNode();
}