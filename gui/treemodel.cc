/*
 *  nextpnr -- Next Generation Place and Route
 *
 *  Copyright (C) 2018  Miodrag Milanovic <miodrag@symbioticeda.com>
 *
 *  Permission to use, copy, modify, and/or distribute this software for any
 *  purpose with or without fee is hereby granted, provided that the above
 *  copyright notice and this permission notice appear in all copies.
 *
 *  THE SOFTWARE IS PROVIDED "AS IS" AND THE AUTHOR DISCLAIMS ALL WARRANTIES
 *  WITH REGARD TO THIS SOFTWARE INCLUDING ALL IMPLIED WARRANTIES OF
 *  MERCHANTABILITY AND FITNESS. IN NO EVENT SHALL THE AUTHOR BE LIABLE FOR
 *  ANY SPECIAL, DIRECT, INDIRECT, OR CONSEQUENTIAL DAMAGES OR ANY DAMAGES
 *  WHATSOEVER RESULTING FROM LOSS OF USE, DATA OR PROFITS, WHETHER IN AN
 *  ACTION OF CONTRACT, NEGLIGENCE OR OTHER TORTIOUS ACTION, ARISING OUT OF
 *  OR IN CONNECTION WITH THE USE OR PERFORMANCE OF THIS SOFTWARE.
 *
 */

#include "treemodel.h"

NEXTPNR_NAMESPACE_BEGIN

ContextTreeItem::ContextTreeItem() { parentNode = nullptr; }

ContextTreeItem::ContextTreeItem(QString name)
        : parentNode(nullptr), itemId(IdString()), itemType(ElementType::NONE), itemName(name)
{
}

ContextTreeItem::ContextTreeItem(IdString id, ElementType type, QString name)
        : parentNode(nullptr), itemId(id), itemType(type), itemName(name)
{
}

ContextTreeItem::~ContextTreeItem()
{
    if (parentNode)
        parentNode->children.removeOne(this);
    qDeleteAll(children);
}
void ContextTreeItem::addChild(ContextTreeItem *item)
{
    item->parentNode = this;
    children.append(item);
}

ContextTreeModel::ContextTreeModel(QObject *parent) : QAbstractItemModel(parent) { root = new ContextTreeItem(); }

ContextTreeModel::~ContextTreeModel() { delete root; }

void ContextTreeModel::loadData(Context *ctx)
{
    if (!ctx)
        return;
    delete root;
    root = new ContextTreeItem();

    for (int i = 0; i < 6; i++)
        nameToItem[i].clear();

    IdString none;

    ContextTreeItem *bels_root = new ContextTreeItem("Bels");
    root->addChild(bels_root);
    QMap<QString, ContextTreeItem *> bel_items;

    // Add bels to tree
    for (auto bel : ctx->getBels()) {
        IdString id = ctx->getBelName(bel);
        QStringList items = QString(id.c_str(ctx)).split("/");
        QString name;
        ContextTreeItem *parent = bels_root;
        for (int i = 0; i < items.size(); i++) {
            if (!name.isEmpty())
                name += "/";
            name += items.at(i);
            if (!bel_items.contains(name)) {
                if (i == items.size() - 1) {
                    ContextTreeItem *item = new ContextTreeItem(id, ElementType::BEL, items.at(i));
                    parent->addChild(item);
                    nameToItem[0].insert(name, item);
                } else {
                    ContextTreeItem *item = new ContextTreeItem(none, ElementType::NONE, items.at(i));
                    parent->addChild(item);
                    bel_items.insert(name, item);
                }
            }
            parent = bel_items[name];
        }
    }

    ContextTreeItem *wire_root = new ContextTreeItem("Wires");
    root->addChild(wire_root);
    QMap<QString, ContextTreeItem *> wire_items;

    // Add wires to tree
    for (auto wire : ctx->getWires()) {
        auto id = ctx->getWireName(wire);
        QStringList items = QString(id.c_str(ctx)).split("/");
        QString name;
        ContextTreeItem *parent = wire_root;
        for (int i = 0; i < items.size(); i++) {
            if (!name.isEmpty())
                name += "/";
            name += items.at(i);
            if (!wire_items.contains(name)) {
                if (i == items.size() - 1) {
                    ContextTreeItem *item = new ContextTreeItem(id, ElementType::WIRE, items.at(i));
                    parent->addChild(item);
                    nameToItem[1].insert(name, item);
                } else {
                    ContextTreeItem *item = new ContextTreeItem(none, ElementType::NONE, items.at(i));
                    parent->addChild(item);
                    wire_items.insert(name, item);
                }
            }
            parent = wire_items[name];
        }
    }

    ContextTreeItem *pip_root = new ContextTreeItem("Pips");
    root->addChild(pip_root);
    QMap<QString, ContextTreeItem *> pip_items;

    // Add pips to tree
    for (auto pip : ctx->getPips()) {
        auto id = ctx->getPipName(pip);
        QStringList items = QString(id.c_str(ctx)).split("/");
        QString name;
        ContextTreeItem *parent = pip_root;
        for (int i = 0; i < items.size(); i++) {
            if (!name.isEmpty())
                name += "/";
            name += items.at(i);
            if (!pip_items.contains(name)) {
                if (i == items.size() - 1) {
                    ContextTreeItem *item = new ContextTreeItem(id, ElementType::PIP, items.at(i));
                    parent->addChild(item);
                    nameToItem[2].insert(name, item);
                } else {
                    ContextTreeItem *item = new ContextTreeItem(none, ElementType::NONE, items.at(i));
                    parent->addChild(item);
                    pip_items.insert(name, item);
                }
            }
            parent = pip_items[name];
        }
    }

    nets_root = new ContextTreeItem("Nets");
    root->addChild(nets_root);

    cells_root = new ContextTreeItem("Cells");
    root->addChild(cells_root);
}

void ContextTreeModel::updateData(Context *ctx)
{
    if (!ctx)
        return;

    // Remove nets not existing any more
    QMap<QString, ContextTreeItem *>::iterator i = nameToItem[3].begin();
    while (i != nameToItem[3].end()) {
        QMap<QString, ContextTreeItem *>::iterator prev = i;
        ++i;
        if (ctx->nets.find(ctx->id(prev.key().toStdString())) == ctx->nets.end()) {
            delete prev.value();
            nameToItem[3].erase(prev);
        }
    }
    // Add nets to tree
    for (auto &item : ctx->nets) {
        auto id = item.first;
        QString name = QString(id.c_str(ctx));
        if (!nameToItem[3].contains(name)) {
            ContextTreeItem *newItem = new ContextTreeItem(id, ElementType::NET, name);
            nets_root->addChild(newItem);
            nameToItem[3].insert(name, newItem);
        }
    }

    // Remove cells not existing any more
    i = nameToItem[4].begin();
    while (i != nameToItem[4].end()) {
        QMap<QString, ContextTreeItem *>::iterator prev = i;
        ++i;
        if (ctx->cells.find(ctx->id(prev.key().toStdString())) == ctx->cells.end()) {
            delete prev.value();
            nameToItem[4].erase(prev);
        }
    }
    // Add cells to tree
    for (auto &item : ctx->cells) {
        auto id = item.first;
        QString name = QString(id.c_str(ctx));
        if (!nameToItem[4].contains(name)) {
            ContextTreeItem *newItem = new ContextTreeItem(id, ElementType::CELL, name);
            cells_root->addChild(newItem);
            nameToItem[4].insert(name, newItem);
        }
    }
}

int ContextTreeModel::rowCount(const QModelIndex &parent) const { return nodeFromIndex(parent)->count(); }

int ContextTreeModel::columnCount(const QModelIndex &parent) const { return 1; }

QModelIndex ContextTreeModel::index(int row, int column, const QModelIndex &parent) const
{
    ContextTreeItem *node = nodeFromIndex(parent);
    if (row >= node->count())
        return QModelIndex();
    return createIndex(row, column, node->at(row));
}

QModelIndex ContextTreeModel::parent(const QModelIndex &child) const
{
    ContextTreeItem *parent = nodeFromIndex(child)->parent();
    if (parent == root)
        return QModelIndex();
    ContextTreeItem *node = parent->parent();
    return createIndex(node->indexOf(parent), 0, parent);
}

QVariant ContextTreeModel::data(const QModelIndex &index, int role) const
{
    if (index.column() != 0)
        return QVariant();
    if (role != Qt::DisplayRole)
        return QVariant();
    ContextTreeItem *node = nodeFromIndex(index);
    return node->name();
}

QVariant ContextTreeModel::headerData(int section, Qt::Orientation orientation, int role) const
{
    Q_UNUSED(section);
    if (orientation == Qt::Horizontal && role == Qt::DisplayRole)
        return QString("Items");

    return QVariant();
}

ContextTreeItem *ContextTreeModel::nodeFromIndex(const QModelIndex &idx) const
{
    if (idx.isValid())
        return (ContextTreeItem *)idx.internalPointer();
    return root;
}

static int getElementIndex(ElementType type)
{
    if (type == ElementType::BEL)
        return 0;
    if (type == ElementType::WIRE)
        return 1;
    if (type == ElementType::PIP)
        return 2;
    if (type == ElementType::NET)
        return 3;
    if (type == ElementType::CELL)
        return 4;
    return -1;
}

ContextTreeItem *ContextTreeModel::nodeForIdType(const ElementType type, const QString name) const
{
    int index = getElementIndex(type);
    if (type != ElementType::NONE && nameToItem[index].contains(name))
        return nameToItem[index].value(name);
    return nullptr;
}

QModelIndex ContextTreeModel::indexFromNode(ContextTreeItem *node)
{
    ContextTreeItem *parent = node->parent();
    if (parent == root)
        return QModelIndex();
    return createIndex(parent->indexOf(node), 0, node);
}

Qt::ItemFlags ContextTreeModel::flags(const QModelIndex &index) const
{
    ContextTreeItem *node = nodeFromIndex(index);
    return Qt::ItemIsEnabled | (node->type() != ElementType::NONE ? Qt::ItemIsSelectable : Qt::NoItemFlags);
}
NEXTPNR_NAMESPACE_END