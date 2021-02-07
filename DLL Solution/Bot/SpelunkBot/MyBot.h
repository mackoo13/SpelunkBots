#pragma once
#include "IBot.h"
#include <vector>

class Node {
public:
	int x;
	int y;
	Node() {}
	Node(int paramX, int paramY) {
		x = paramX;
		y = paramY;
	}
};

class Action {
public:
	Node from;
	Node to;
	Node delta;

	Node Progress(Node current) {
		return *(new Node(to.x - current.x, to.y - current.y));
	}
};

class MyBot : public IBot
{
public:
	void Update() override;
	void Initialise();
	void Stop();
	void Log();
	bool WantsToLookAround();
	bool IsDiscovered(int x, int y);
	void LookAround();
	void BuildGraph(bool save);
	std::vector<Node> GetNeighbours(int x, int y);
	double NodeOffsetX();
	double NodeOffsetY();
	Node GetCurrentNode();
	bool MyBot::IsPassthroughNode(int x, int y);
	bool MyBot::IsSolidNode(int x, int y);
	bool CanStandOn(int x, int y);
	bool CanStandOn(Node node);
	bool CanFallFrom(int x, int y);
	bool StandsOnGround();

	// Actions
	bool MyBot::GoToNodeCenter();

	// Debug
	void SaveMap();
	void SaveNeighbours();
};