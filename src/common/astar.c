/*
Copyright (c) 2012, Sean Heber. All rights reserved.

Redistribution and use in source and binary forms, with or without
modification, are permitted provided that the following conditions are met:

1. Redistributions of source code must retain the above copyright
notice, this list of conditions and the following disclaimer.

2. Redistributions in binary form must reproduce the above copyright notice,
this list of conditions and the following disclaimer in the documentation
and/or other materials provided with the distribution.

3. Neither the name of Sean Heber nor the names of its contributors may
be used to endorse or promote products derived from this software without
specific prior written permission.

THIS SOFTWARE IS PROVIDED BY THE COPYRIGHT HOLDERS AND CONTRIBUTORS "AS IS" AND
ANY EXPRESS OR IMPLIED WARRANTIES, INCLUDING, BUT NOT LIMITED TO, THE IMPLIED
WARRANTIES OF MERCHANTABILITY AND FITNESS FOR A PARTICULAR PURPOSE ARE
DISCLAIMED. IN NO EVENT SHALL SEAN HEBER BE LIABLE FOR ANY DIRECT,
INDIRECT, INCIDENTAL, SPECIAL, EXEMPLARY, OR CONSEQUENTIAL DAMAGES (INCLUDING,
BUT NOT LIMITED TO, PROCUREMENT OF SUBSTITUTE GOODS OR SERVICES; LOSS OF USE,
DATA, OR PROFITS; OR BUSINESS INTERRUPTION) HOWEVER CAUSED AND ON ANY THEORY OF
LIABILITY, WHETHER IN CONTRACT, STRICT LIABILITY, OR TORT (INCLUDING NEGLIGENCE
OR OTHERWISE) ARISING IN ANY WAY OUT OF THE USE OF THIS SOFTWARE, EVEN IF
ADVISED OF THE POSSIBILITY OF SUCH DAMAGE.
*/

#include "shared/shared.h"
#include "common/astar.h"
#include "common/zone.h"
#include <math.h>
#include <string.h>

struct __ASNeighborList
{
	const ASPathNodeSource *source;
	size_t capacity;
	size_t count;
	ASGoalType *costs;
	byte *nodeKeys;
};

struct __ASPath
{
	size_t nodeSize;
	size_t count;
	ASGoalType cost;
};

typedef enum
{
	isClosed = 1,
	isOpen = 2,
	isGoal = 4,
	hasParent = 8,
	hasEstimatedCost = 16
} NodeRecordFlags;

typedef struct
{
	NodeRecordFlags flags;
	ASGoalType estimatedCost;
	ASGoalType cost;
	size_t openIndex;
	size_t parentIndex;
} NodeRecord;

#define RECORD_HAS_FLAG(record, flag) (record->flags & flag)
#define RECORD_ADD_FLAG(record, flag) (record->flags |= flag)
#define RECORD_REMOVE_FLAG(record, flag) (record->flags &= ~flag)

struct __VisitedNodes
{
	const ASPathNodeSource *source;
	void *context;
	size_t nodeRecordsCapacity;
	size_t nodeRecordsCount;
	byte *nodeRecords;
	size_t *nodeRecordsIndex;           // array of nodeRecords indexes, kept sorted by nodeRecords[i]->nodeKey using source->nodeComparator
	size_t openNodesCapacity;
	size_t openNodesCount;
	size_t *openNodes;                  // binary heap of nodeRecords indexes, sorted by the nodeRecords[i]->rank
};
typedef struct __VisitedNodes *VisitedNodes;

typedef struct
{
	VisitedNodes nodes;
	size_t index;
} Node;

static const Node NodeNull = { NULL, (uint32_t) -1 };

/********************************************/

static inline VisitedNodes VisitedNodesCreate(const ASPathNodeSource *source, void *context)
{
	VisitedNodes nodes = (VisitedNodes)Z_Mallocz(sizeof(struct __VisitedNodes));
	nodes->source = source;
	nodes->context = context;
	return nodes;
}

static inline void VisitedNodesDestroy(VisitedNodes visitedNodes)
{
	Z_Free(visitedNodes->nodeRecordsIndex);
	Z_Free(visitedNodes->nodeRecords);
	Z_Free(visitedNodes->openNodes);
	Z_Free(visitedNodes);
}

static inline int NodeIsNull(Node n)
{
	return (n.nodes == NodeNull.nodes) && (n.index == NodeNull.index);
}

static inline Node NodeMake(VisitedNodes nodes, size_t index)
{
	return (Node)
	{
		nodes, index
	};
}

static inline NodeRecord *NodeGetRecord(Node node)
{
	return (NodeRecord *)(node.nodes->nodeRecords + (node.index * (node.nodes->source->nodeSize + sizeof(NodeRecord))));
}

static inline void *GetNodeKey(Node node)
{
	return NodeGetRecord(node) + 1;
}

static inline int NodeIsInOpenSet(Node n)
{
	return RECORD_HAS_FLAG(NodeGetRecord(n), isOpen);
}

static inline int NodeIsInClosedSet(Node n)
{
	return RECORD_HAS_FLAG(NodeGetRecord(n), isClosed);
}

static inline void RemoveNodeFromClosedSet(Node n)
{
	RECORD_REMOVE_FLAG(NodeGetRecord(n), isClosed);
}

static inline void AddNodeToClosedSet(Node n)
{
	RECORD_ADD_FLAG(NodeGetRecord(n), isClosed);
}

static inline ASGoalType GetNodeEstimatedCost(Node n)
{
	return NodeGetRecord(n)->estimatedCost;
}

static inline void SetNodeEstimatedCost(Node n, ASGoalType estimatedCost)
{
	NodeRecord *record = NodeGetRecord(n);
	record->estimatedCost = estimatedCost;
	RECORD_ADD_FLAG(record, hasEstimatedCost);
}

static inline ASGoalType GetNodeRank(Node n)
{
	NodeRecord *record = NodeGetRecord(n);
	return GetNodeEstimatedCost(n) + record->cost;
}

static inline ASGoalType GetNodeCost(Node n)
{
	return NodeGetRecord(n)->cost;
}

static inline int NodeHasEstimatedCost(Node n)
{
	return RECORD_HAS_FLAG(NodeGetRecord(n), hasEstimatedCost);
}

static inline void SetNodeIsGoal(Node n)
{
	if (!NodeIsNull(n))
		RECORD_ADD_FLAG(NodeGetRecord(n), isGoal);
}

static inline int NodeIsGoal(Node n)
{
	return !NodeIsNull(n) && RECORD_HAS_FLAG(NodeGetRecord(n), isGoal);
}

static inline Node GetParentNode(Node n)
{
	NodeRecord *record = NodeGetRecord(n);

	if (RECORD_HAS_FLAG(record, hasParent))
		return NodeMake(n.nodes, record->parentIndex);
	else
		return NodeNull;
}

static inline int NodeRankCompare(Node n1, Node n2)
{
	const ASGoalType rank1 = GetNodeRank(n1);
	const ASGoalType rank2 = GetNodeRank(n2);

	if (rank1 < rank2)
		return -1;
	else if (rank1 > rank2)
		return 1;
	else
		return 0;
}

static inline ASGoalType GetPathCostHeuristic(Node a, Node b)
{
	if (a.nodes->source->pathCostHeuristic && !NodeIsNull(a) && !NodeIsNull(b))
		return a.nodes->source->pathCostHeuristic(GetNodeKey(a), GetNodeKey(b), a.nodes->context);
	else
		return 0;
}

static inline int NodeKeyCompare(Node node, void *nodeKey)
{
	if (node.nodes->source->nodeComparator)
		return node.nodes->source->nodeComparator(GetNodeKey(node), nodeKey, node.nodes->context);
	else
		return memcmp(GetNodeKey(node), nodeKey, node.nodes->source->nodeSize);
}

static inline Node GetNode(VisitedNodes nodes, void *nodeKey)
{
	if (!nodeKey)
		return NodeNull;

	// looks it up in the index, if it's not found it inserts a new record in the sorted index and the nodeRecords array and returns a reference to it
	size_t first = 0;

	if (nodes->nodeRecordsCount > 0)
	{
		size_t last = nodes->nodeRecordsCount - 1;

		while (first <= last)
		{
			const size_t mid = (first + last) / 2;
			const int comp = NodeKeyCompare(NodeMake(nodes, nodes->nodeRecordsIndex[mid]), nodeKey);

			if (comp < 0)
				first = mid + 1;
			else if (comp > 0 && mid > 0)
				last = mid - 1;
			else if (comp > 0)
				break;
			else
				return NodeMake(nodes, nodes->nodeRecordsIndex[mid]);
		}
	}

	if (nodes->nodeRecordsCount == nodes->nodeRecordsCapacity)
	{
		nodes->nodeRecordsCapacity = 1 + (nodes->nodeRecordsCapacity * 2);
		nodes->nodeRecords = (byte *)Z_Realloc(nodes->nodeRecords, nodes->nodeRecordsCapacity * (sizeof(NodeRecord) + nodes->source->nodeSize));
		nodes->nodeRecordsIndex = (size_t *)Z_Realloc(nodes->nodeRecordsIndex, nodes->nodeRecordsCapacity * sizeof(size_t));
	}

	Node node = NodeMake(nodes, nodes->nodeRecordsCount);
	nodes->nodeRecordsCount++;
	memmove(&nodes->nodeRecordsIndex[first + 1], &nodes->nodeRecordsIndex[first], (nodes->nodeRecordsCapacity - first - 1) * sizeof(size_t));
	nodes->nodeRecordsIndex[first] = node.index;
	NodeRecord *record = NodeGetRecord(node);
	memset(record, 0, sizeof(NodeRecord));
	memcpy((record + 1), nodeKey, nodes->source->nodeSize);
	return node;
}

static inline void SwapOpenSetNodesAtIndexes(VisitedNodes nodes, size_t index1, size_t index2)
{
	if (index1 != index2)
	{
		NodeRecord *record1 = NodeGetRecord(NodeMake(nodes, nodes->openNodes[index1]));
		NodeRecord *record2 = NodeGetRecord(NodeMake(nodes, nodes->openNodes[index2]));
		const size_t tempOpenIndex = record1->openIndex;
		record1->openIndex = record2->openIndex;
		record2->openIndex = tempOpenIndex;
		const size_t tempNodeIndex = nodes->openNodes[index1];
		nodes->openNodes[index1] = nodes->openNodes[index2];
		nodes->openNodes[index2] = tempNodeIndex;
	}
}

static inline void DidRemoveFromOpenSetAtIndex(VisitedNodes nodes, size_t index)
{
	size_t smallestIndex = index;

	do
	{
		if (smallestIndex != index)
		{
			SwapOpenSetNodesAtIndexes(nodes, smallestIndex, index);
			index = smallestIndex;
		}

		const size_t leftIndex = (2 * index) + 1;
		const size_t rightIndex = (2 * index) + 2;

		if (leftIndex < nodes->openNodesCount && NodeRankCompare(NodeMake(nodes, nodes->openNodes[leftIndex]), NodeMake(nodes, nodes->openNodes[smallestIndex])) < 0)
			smallestIndex = leftIndex;

		if (rightIndex < nodes->openNodesCount && NodeRankCompare(NodeMake(nodes, nodes->openNodes[rightIndex]), NodeMake(nodes, nodes->openNodes[smallestIndex])) < 0)
			smallestIndex = rightIndex;
	}
	while (smallestIndex != index);
}

static inline void RemoveNodeFromOpenSet(Node n)
{
	NodeRecord *record = NodeGetRecord(n);

	if (RECORD_HAS_FLAG(record, isOpen))
	{
		RECORD_REMOVE_FLAG(record, isOpen);
		n.nodes->openNodesCount--;
		const size_t index = record->openIndex;
		SwapOpenSetNodesAtIndexes(n.nodes, index, n.nodes->openNodesCount);
		DidRemoveFromOpenSetAtIndex(n.nodes, index);
	}
}

static inline void DidInsertIntoOpenSetAtIndex(VisitedNodes nodes, size_t index)
{
	while (index > 0)
	{
		const size_t parentIndex = floorf((index - 1) / 2);

		if (NodeRankCompare(NodeMake(nodes, nodes->openNodes[parentIndex]), NodeMake(nodes, nodes->openNodes[index])) < 0)
			break;
		else
		{
			SwapOpenSetNodesAtIndexes(nodes, parentIndex, index);
			index = parentIndex;
		}
	}
}

static inline void AddNodeToOpenSet(Node n, ASGoalType cost, Node parent)
{
	NodeRecord *record = NodeGetRecord(n);

	if (!NodeIsNull(parent))
	{
		RECORD_ADD_FLAG(record, hasParent);
		record->parentIndex = parent.index;
	}
	else
		RECORD_REMOVE_FLAG(record, hasParent);

	if (n.nodes->openNodesCount == n.nodes->openNodesCapacity)
	{
		n.nodes->openNodesCapacity = 1 + (n.nodes->openNodesCapacity * 2);
		n.nodes->openNodes = (size_t *)Z_Realloc(n.nodes->openNodes, n.nodes->openNodesCapacity * sizeof(size_t));
	}

	const size_t openIndex = n.nodes->openNodesCount;
	n.nodes->openNodes[openIndex] = n.index;
	n.nodes->openNodesCount++;
	record->openIndex = openIndex;
	RECORD_ADD_FLAG(record, isOpen);
	record->cost = cost;
	DidInsertIntoOpenSetAtIndex(n.nodes, openIndex);
}

static inline int HasOpenNode(VisitedNodes nodes)
{
	return nodes->openNodesCount > 0;
}

static inline Node GetOpenNode(VisitedNodes nodes)
{
	return NodeMake(nodes, nodes->openNodes[0]);
}

static inline ASNeighborList NeighborListCreate(const ASPathNodeSource *source)
{
	ASNeighborList list = (ASNeighborList)Z_Mallocz(sizeof(struct __ASNeighborList));
	list->source = source;
	return list;
}

static inline void NeighborListDestroy(ASNeighborList list)
{
	Z_Free(list->costs);
	Z_Free(list->nodeKeys);
	Z_Free(list);
}

static inline ASGoalType NeighborListGetEdgeCost(ASNeighborList list, size_t index)
{
	return list->costs[index];
}

static void *NeighborListGetNodeKey(ASNeighborList list, size_t index)
{
	return list->nodeKeys + (index * list->source->nodeSize);
}

/********************************************/

void ASNeighborListAdd(ASNeighborList list, void *node, ASGoalType edgeCost)
{
	if (list->count == list->capacity)
	{
		list->capacity = 1 + (list->capacity * 2);
		list->costs = (ASGoalType *)Z_Realloc(list->costs, sizeof(ASGoalType) * list->capacity);
		list->nodeKeys = (byte *)Z_Realloc(list->nodeKeys, list->source->nodeSize * list->capacity);
	}

	list->costs[list->count] = edgeCost;
	memcpy(list->nodeKeys + (list->count * list->source->nodeSize), node, list->source->nodeSize);
	list->count++;
}

ASPath ASPathCreate(const ASPathNodeSource *source, void *context, void *startNodeKey, void *goalNodeKey)
{
	if (!startNodeKey || !source || !source->nodeNeighbors || source->nodeSize == 0)
		return NULL;

	VisitedNodes visitedNodes = VisitedNodesCreate(source, context);
	ASNeighborList neighborList = NeighborListCreate(source);
	Node current = GetNode(visitedNodes, startNodeKey);
	Node goalNode = GetNode(visitedNodes, goalNodeKey);
	ASPath path = NULL;
	// mark the goal node as the goal
	SetNodeIsGoal(goalNode);
	// set the starting node's estimate cost to the goal and add it to the open set
	SetNodeEstimatedCost(current, GetPathCostHeuristic(current, goalNode));
	AddNodeToOpenSet(current, 0, NodeNull);

	// perform the A* algorithm
	while (HasOpenNode(visitedNodes) && !NodeIsGoal((current = GetOpenNode(visitedNodes))))
	{
		if (source->earlyExit)
		{
			const int shouldExit = source->earlyExit(visitedNodes->nodeRecordsCount, GetNodeKey(current), goalNodeKey, context);

			if (shouldExit > 0)
			{
				SetNodeIsGoal(current);
				break;
			}
			else if (shouldExit < 0)
				break;
		}

		RemoveNodeFromOpenSet(current);
		AddNodeToClosedSet(current);
		neighborList->count = 0;
		source->nodeNeighbors(neighborList, GetNodeKey(current), context);

		for (size_t n = 0; n < neighborList->count; n++)
		{
			const ASGoalType cost = GetNodeCost(current) + NeighborListGetEdgeCost(neighborList, n);
			Node neighbor = GetNode(visitedNodes, NeighborListGetNodeKey(neighborList, n));

			if (!NodeHasEstimatedCost(neighbor))
				SetNodeEstimatedCost(neighbor, GetPathCostHeuristic(neighbor, goalNode));

			if (NodeIsInOpenSet(neighbor) && cost < GetNodeCost(neighbor))
				RemoveNodeFromOpenSet(neighbor);

			if (NodeIsInClosedSet(neighbor) && cost < GetNodeCost(neighbor))
				RemoveNodeFromClosedSet(neighbor);

			if (!NodeIsInOpenSet(neighbor) && !NodeIsInClosedSet(neighbor))
				AddNodeToOpenSet(neighbor, cost, current);
		}
	}

	if (NodeIsNull(goalNode))
		SetNodeIsGoal(current);

	if (NodeIsGoal(current))
	{
		size_t count = 0;
		Node n = current;

		while (!NodeIsNull(n))
		{
			count++;
			n = GetParentNode(n);
		}

		path = (ASPath)Z_Malloc(sizeof(struct __ASPath) + (count * source->nodeSize));
		path->nodeSize = source->nodeSize;
		path->count = count;
		path->cost = GetNodeCost(current);
		n = current;

		for (size_t i = count; i > 0; i--)
		{
			memcpy((byte *)(path + 1) + ((i - 1) * source->nodeSize), GetNodeKey(n), source->nodeSize);
			n = GetParentNode(n);
		}
	}

	NeighborListDestroy(neighborList);
	VisitedNodesDestroy(visitedNodes);
	return path;
}

void ASPathDestroy(ASPath path)
{
	Z_Free(path);
}

ASPath ASPathCopy(ASPath path)
{
	if (path)
	{
		const size_t size = sizeof(struct __ASPath) + (path->count * path->nodeSize);
		ASPath newPath = (ASPath)Z_Malloc(size);
		memcpy(newPath, path, size);
		return newPath;
	}
	else
		return NULL;
}

ASGoalType ASPathGetCost(ASPath path)
{
	return path ? path->cost : (ASGoalType)INFINITY;
}

size_t ASPathGetCount(ASPath path)
{
	return path ? path->count : 0;
}

void *ASPathGetNode(ASPath path, size_t index)
{
	return (path && index < path->count) ? ((byte *)(path + 1) + (index * path->nodeSize)) : NULL;
}