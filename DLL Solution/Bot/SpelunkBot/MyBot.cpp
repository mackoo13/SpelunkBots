#include "stdafx.h"
#include "MyBot.h"
#include <iostream>
#include <fstream>
#include <string> 
#include <vector>
#include <stdlib.h>
#include <time.h>  
#include <chrono>
#include <thread>

int X_SIGHT = 5;
int Y_SIGHT = 5;
int SAFE_FALL = 5;
int MAX_LADDER_HEIGHT = 5;
int ARROW_TRAP_RANGE = 7;

std::string OBJECT_NAMES[68] = { 
	"", "goldBar", "goldBars", "emerald", "", "", "", "", "", "",
	"", "", "", "", "", "", "", "", "", "", 
	"", "", "", "", "", "", "", "", "", "", 
	"", "", "", "", "", "", "", "", "", "", 
	"", "", "", "", "", "", "", "", "", "", 
	"", "", "", "", "", "", "", "", "", "", 
	"", "", "", "", "", "block", "pushBlock", ""
};






std::ofstream logFile;

std::string nodeChars = "? #HEE5><SIM+";

bool initialised = false;
bool busy = false;
int skipNext = 0;
int map[X_NODES+1][Y_NODES+1];
bool safe[X_NODES+1][Y_NODES+1];
bool visited[X_NODES+1][Y_NODES+1];
std::vector<Node> neighbours[X_NODES+1][Y_NODES+1];
std::vector<int> items[X_NODES+1][Y_NODES+1];
std::vector<int> enemies[X_NODES+1][Y_NODES+1];
Action action;



void MyBot::Update()
{
	if (!initialised) {
		Initialise();
		initialised = true;
	}

	Stop();
	busy = false;

	if (skipNext > 0) {
		Log();
		skipNext--;
		return;
	}

	

	if (_hasGoal) {
		int deltaX = (int)_targetX - (int)floor(_playerPositionXNode);
		int deltaY = (int)_targetY - (int)floor(_playerPositionYNode);

		if (deltaX == 0) {
			busy = GoToNodeCenter();
		}

		if (!busy) {
			if (map[(int)floor(_playerPositionXNode)][(int)floor(_playerPositionYNode)] == spLadder) {
				if (deltaX == 0 && deltaY == -1) {
					_lookUp = true;
					busy = true;
				}
				else if (deltaX == 0 && deltaY == 1) {
					//_lookUp = true;
					_duck = true;
					_goRight = true;
					busy = true;
				}
			}
		}

		if(!busy) {
			if (deltaX == -1 && (deltaY >= 0 && deltaY <= SAFE_FALL)) {
				_goLeft = true;
			}
			else if (deltaX == 1 && (deltaY >= 0 && deltaY <= SAFE_FALL)) {
				_goRight = true;
			}
			else if ((deltaX == -1 || deltaX == -2) && (deltaY == -1 || deltaY == -2)) {
				//if (!_spIsInAir && NodeOffsetX() < .5) {
					_jump = true;
				//}
				if (IsSolidNode((int)_targetX-1, (int)_targetY)) {
					_goLeft = true;
				}
			}
			else if ((deltaX == 1 || deltaX == 2) && (deltaY == -1 || deltaY == -2)) {
				//if (!_spIsInAir && NodeOffsetX() > .5) {
					_jump = true;
				//}
				if (IsSolidNode((int)_targetX + 1, (int)_targetY)) {
					_goRight = true;
				}
			}
		}
		
		if (deltaX == 0 && deltaY == 0) {
			//Stop();
			std::vector<Node> ns = neighbours[(int)_targetX][(int)_targetY];
			if (!ns.empty()) {
				Node newTarget = ns[rand() % ns.size()];
				_targetX = newTarget.x;
				_targetY = newTarget.y;

				for (size_t i = 0; i < ns.size(); i++) {
					if (!visited[ns[i].x][ns[i].y]) {
						_targetX = ns[i].x;
						_targetY = ns[i].y;
					}
				}
			}

			if (StandsOnGround() && WantsToLookAround()) {
				LookAround();
				busy = true;
			}

			visited[(int)_targetX][(int)_targetY] = true;
			skipNext = 5;
		}

		Log();
	}
}

void MyBot::Initialise() {
	logFile.open("logs/log.txt");

	for (int y = 0; y < Y_NODES; y++) {
		for (int x = 0; x < X_NODES; x++) {
			map[x][y] = -1;
			safe[x][y] = true;
			visited[x][y] = false;
		}
	}

	LookAround();

	/*neighbours[4][4] = *(new Node(5, 3));
	neighbours[5][3] = *(new Node(6, 4));
	neighbours[6][4] = *(new Node(7, 4));
	neighbours[7][4] = *(new Node(8, 4));
	neighbours[8][4] = *(new Node(9, 4));
	neighbours[9][4] = *(new Node(11, 2));*/

	/*neighbours[4][4] = *(new Node(3, 4));
	neighbours[3][4] = *(new Node(2, 7));
	neighbours[2][7] = *(new Node(3, 7));
	neighbours[3][7] = *(new Node(4, 7));
	neighbours[4][7] = *(new Node(5, 7));
	neighbours[5][7] = *(new Node(6, 12));*/

	Node current = GetCurrentNode();
	_targetX = current.x;
	_targetY = current.y;
	_hasGoal = true;

	srand(time(NULL));
	
}

void MyBot::Stop() {
	_goLeft = false;
	_goRight = false;
	_jump = false;
	_duck = false;
}

void MyBot::Log() {
	logFile << std::to_string(_playerPositionXNode)
		<< "\t"
		<< std::to_string(_playerPositionYNode)
		<< "\t"
		<< std::to_string(_targetX)
		<< "\t"
		<< std::to_string(_targetY)
		<< "\t"
		<< (_goLeft ? "left " : "")
		<< (_goRight ? "right " : "")
		<< (_jump ? "jump " : "")
		<< (_duck ? "duck " : "")
		<< (skipNext > 0 ? "skip " : "")
		<< "\n";
}

// Exploration

bool MyBot::WantsToLookAround() {
	return !IsDiscovered((int)floor(_playerPositionXNode) + X_SIGHT, (int)floor(_playerPositionYNode) + Y_SIGHT);
	// todo inne kierunki
}

bool MyBot::IsDiscovered(int x, int y) {
	return x<0 || y<0 || x>=X_NODES || y>=Y_NODES || map[x][y] != -1;
}

void MyBot::LookAround() {
	logFile << "LookAround\n";

	std::vector<Node> arrowTraps;

	for (int y = 0; y < Y_NODES; y++) {
		for (int x = 0; x < X_NODES; x++) {
			if (!IsDiscovered(x, y)) {
				map[x][y] = (int)GetNodeState(x, y, NODE_COORDS);
				if (GetIDOfCollectableInNode(spBlock, x, y, NODE_COORDS) != 0 || GetIDOfCollectableInNode(spPushBlock, x, y, NODE_COORDS) != 0) {
					map[x][y] = spStandardTerrain;
				}

				for (int itemType = 0; itemType < 68; itemType++) {
					int itemId = GetIDOfCollectableInNode(itemType, x, y, NODE_COORDS);
					if (itemId != 0) {
						items[x][y].push_back(itemType);
					}
				}

				for (int enemyType = 0; enemyType < 100; enemyType++) {
					int enemyId = GetIDOfEnemyInNode(enemyType, x, y, NODE_COORDS);
					if (enemyId != 0) {
						enemies[x][y].push_back(enemyType);
					}
				}

				if (map[x][y] == spArrowTrapLeft || map[x][y] == spArrowTrapRight) {
					arrowTraps.push_back(*(new Node(x, y)));
				}
			}
		}
	}

	for (size_t i = 0; i < arrowTraps.size(); i++) {
		int x = arrowTraps[i].x;
		int y = arrowTraps[i].y;

		int dir = map[x][y] == spArrowTrapLeft ? -1 : 1;
		
		for (int dx = 1; dx < ARROW_TRAP_RANGE; dx++) {
			if (!IsPassthroughNode(x + dx * dir, y)) {
				break;
			}

			safe[x + dx * dir][y] = false;
		}
	}

	BuildGraph(true);
	SaveMap();

}

// Layout

double MyBot::NodeOffsetX() {
	return _playerPositionXNode - floor(_playerPositionXNode);
}

double MyBot::NodeOffsetY() {
	return _playerPositionYNode - floor(_playerPositionYNode);
}

Node MyBot::GetCurrentNode() {
	return *(new Node((int)floor(_playerPositionXNode), (int)floor(_playerPositionYNode)));
}

bool MyBot::IsPassthroughNode(int x, int y) {
	return map[x][y] == spEmptyNode
		|| map[x][y] == spLadder
		|| map[x][y] == spEntrance
		|| map[x][y] == spExit;
}

bool MyBot::IsSolidNode(int x, int y) {
	return map[x][y] == spStandardTerrain;
}

bool MyBot::CanStandOn(int x, int y) {
	if(IsPassthroughNode(x, y) && IsSolidNode(x, y+1)) return true;
	if(map[x][y] == spLadder && map[x][y+1] == spLadder && map[x][y-1] != spLadder) return true;
	return false;
}

bool MyBot::CanStandOn(Node node) {
	return CanStandOn(node.x, node.y);
}

bool MyBot::CanFallFrom(int x, int y) {
	return map[x][y] == spEmptyNode
		&& (map[x][y + 1] == spEmptyNode || map[x][y + 1] == spEntrance || map[x][y + 1] == spExit);
}

bool MyBot::StandsOnGround() {
	return abs(_playerPositionYNode - floor(_playerPositionYNode) - .5) < .01;
}

std::vector<Node> MyBot::GetNeighbours(int x, int y) {
	std::vector<Node> res;
	
	if (map[x][y] == -1) return res;

	// Ladder climbing
	if (map[x][y] == spLadder) {
		if (map[x][y - 1] == spLadder) {
			res.push_back(*(new Node(x, y - 1)));
		}
		if (map[x][y + 1] == spLadder) {
			res.push_back(*(new Node(x, y + 1)));
		}
	}
	
	// Walk
	if (!CanStandOn(x, y)) return res;

	for (int dx = -1; dx <= 1; dx += 2) {
		if (CanStandOn(x+dx, y)) {
			res.push_back(*(new Node(x + dx, y)));
		}
		else if (CanFallFrom(x + dx, y)) {
			for (int dy = 0; dy < SAFE_FALL; dy++) {
				if (!safe[x+dx][y+dy]) {
					break;
				}

				if (CanStandOn(x + dx, y+dy)) {
					res.push_back(*(new Node(x + dx, y+dy)));
					break;
				}
			}
		}
	}

	// Jump
	if (map[x][y - 1] == spEmptyNode) {
		for (int dx = -1; dx <= 1; dx += 2) {
			if (CanStandOn(x + dx, y - 1)) {
				res.push_back(*(new Node(x + dx, y - 1)));
			}
			if (IsPassthroughNode(x, y-1) && CanStandOn(x + dx, y - 2)) {
				res.push_back(*(new Node(x + dx, y - 2)));
			}
		}
	}
	

	return res;
}

void MyBot::BuildGraph(bool save) {
	for (int y = 0; y < Y_NODES; y++) {
		for (int x = 0; x < X_NODES; x++) {
			neighbours[x][y] = GetNeighbours(x, y);
		}
	}

	if (save) SaveNeighbours();
}

// Actions

bool MyBot::GoToNodeCenter() {
	if (NodeOffsetX() < .27) {
		_goRight = true;
		return true;
	}
	else if (NodeOffsetX() > .73) {
		_goLeft = true;
		return true;
	}
	
	return false;
}

// Debug

void MyBot::SaveMap() {
	std::ofstream fout;
	fout.open("logs/map.txt");

	for (int y = 0; y < Y_NODES; y++) {
		for (int x = 0; x < X_NODES; x++) {
			fout << nodeChars[map[x][y] + 1];
		}
		fout << "\n";
	}

	fout << "\n\n";

	for (int y = 0; y < Y_NODES; y++) {
		for (int x = 0; x < X_NODES; x++) {
			fout << (safe[x][y] ? "." : "X");
		}
		fout << "\n";
	}

	fout << "\n\n";

	for (int y = 0; y < Y_NODES; y++) {
		for (int x = 0; x < X_NODES; x++) {
			for (size_t i = 0; i < items[x][y].size(); i++) {
				fout << x << "\t" << y << "\t" << items[x][y][i] << "\t" << OBJECT_NAMES[items[x][y][i]] << "\n";
			}
		}
	}

	fout << "\n\n";

	for (int y = 0; y < Y_NODES; y++) {
		for (int x = 0; x < X_NODES; x++) {
			for (size_t i = 0; i < enemies[x][y].size(); i++) {
				fout << x << "\t" << y << "\t" << enemies[x][y][i] << "\n";
			}
		}
	}

	fout.close();
}

void MyBot::SaveNeighbours() {
	std::ofstream myfile;
	std::vector<Node> ns;
	myfile.open("logs/neighbours.txt");

	for (int y = 0; y < Y_NODES; y++) {
		for (int x = 0; x < X_NODES; x++) {
			myfile << x << "," << y << "\t";
			ns = neighbours[x][y];
			
			for (size_t i = 0; i < ns.size(); i++) {
				myfile << ns[i].x << "," << ns[i].y << "\t";
			}

			myfile << "\n";
		}
	}

	myfile.close();
}




