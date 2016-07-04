#ifndef CHIG_NODE_TYPE_HPP
#define CHIG_NODE_TYPE_HPP

#pragma once

#include "chig/ImportedModule.hpp"

#include <iterator>
#include <utility>

#include <llvm/IR/Value.h>
#include <llvm/IR/Function.h>
#include <llvm/IR/IRBuilder.h>
#include <llvm/IR/Instructions.h>


namespace chig {

// generic type
struct NodeType {

	virtual ~NodeType() = default;

	std::string name;
	std::string description;

	// inputs and outputs
	std::vector<llvm::Type*> inputTypes;
	std::vector<std::string> inputDescs;
	
	std::vector<llvm::Type*> outputTypes;
	std::vector<std::string> outputDescs;
	
	unsigned int numOutputExecs = 1;

	virtual void codegen(const std::vector<llvm::Value*>& io, llvm::IRBuilder<>* codegenInto, const std::vector<llvm::BasicBlock*>& outputBlocks) = 0;
	
};

struct FunctionCallNodeType : NodeType {

	FunctionCallNodeType(llvm::Function* func, int num_inputs, int numExecOutputs, std::string argDescription, const std::vector<std::string>& iodescs) : function{func} {
		
		numOutputExecs = numExecOutputs;
		
		name = func->getName();
		description = std::move(argDescription);

		// populate inputs and outputs
		inputTypes.resize(num_inputs);
		auto beginningOfOutptus = func->getArgumentList().begin();
		std::advance(beginningOfOutptus, num_inputs);
		
		std::transform(func->getArgumentList().begin(), beginningOfOutptus, inputTypes.begin(), [](auto& arg){return arg.getType();});

		int num_outputs = std::distance(func->getArgumentList().begin(), func->getArgumentList().end()) - num_inputs;

		outputTypes.resize(num_outputs);
		std::transform(beginningOfOutptus, func->getArgumentList().end(), outputTypes.begin(), [](auto& arg){return arg.getType();});

		// copy in the descriptions
		inputDescs.resize(num_inputs);
		std::copy(iodescs.begin(), iodescs.begin() + num_inputs, inputDescs.begin());
		
		outputDescs.resize(num_outputs);
		std::copy(iodescs.begin() + num_inputs, iodescs.end(), outputDescs.begin());
	}

	llvm::Function* function;

	virtual void codegen(const std::vector<llvm::Value*>& io, llvm::IRBuilder<>* codegenInto, const std::vector<llvm::BasicBlock*>& outputBlocks) override {

		auto ret = codegenInto->CreateCall(function, io);
		
		// TODO: better default
		auto sw = codegenInto->CreateSwitch(ret, outputBlocks[0], numOutputExecs); 
		
		for(size_t i = 0; i < outputBlocks.size(); ++i) {
			sw->addCase(llvm::ConstantInt::get(llvm::IntegerType::get(llvm::getGlobalContext(), 32), i), outputBlocks[i]);
		}
		
	}

};

struct IfNodeType : NodeType {
	
	IfNodeType() {
		
		name = "if";
		description = "branch on a bool";
		
		numOutputExecs = 2;
		
		inputTypes = { llvm::Type::getInt1Ty(llvm::getGlobalContext()) };
		inputDescs = { "condition" };
		
		
		
	}
	
	virtual void codegen(const std::vector<llvm::Value*>& io, llvm::IRBuilder<>* codegenInto, const std::vector<llvm::BasicBlock*>& outputBlocks) override {

		codegenInto->CreateCondBr(io[0], outputBlocks[0], outputBlocks[1]);
		
	}
	
};

}

#endif // CHIG_NODE_TYPE_HPP