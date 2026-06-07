#pragma once

#include "Parser.h"
#include "../predictive/PredictiveParser.h"
#include "crow/json.h"

// Recursive Descent Tree
crow::json::wvalue nodeToJson(Node* node);

// Predictive Parser Tree
crow::json::wvalue predictiveTreeToJson(
    ParseTreeNode* node
);