#include "TreeUtils.h"

// ======================================
// RECURSIVE DESCENT TREE
// ======================================

crow::json::wvalue nodeToJson(Node* node)
{
    crow::json::wvalue result;

    if (!node)
        return result;

    result["name"] = node->name;

    crow::json::wvalue children(
        crow::json::type::List
    );

    for (
        size_t i = 0;
        i < node->children.size();
        i++
    )
    {
        children[i] =
            nodeToJson(
                node->children[i]
            );
    }

    result["children"] =
        std::move(children);

    return result;
}

// ======================================
// PREDICTIVE PARSER TREE
// ======================================

crow::json::wvalue predictiveTreeToJson(
    ParseTreeNode* node
)
{
    crow::json::wvalue result;

    if (!node)
        return result;

    result["name"] =
        node->name;

    crow::json::wvalue children(
        crow::json::type::List
    );

    for (
        size_t i = 0;
        i < node->children.size();
        i++
    )
    {
        children[i] =
            predictiveTreeToJson(
                node->children[i]
            );
    }

    result["children"] =
        std::move(children);

    return result;
}