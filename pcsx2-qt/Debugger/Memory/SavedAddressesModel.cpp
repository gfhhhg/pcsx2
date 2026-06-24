// SPDX-FileCopyrightText: 2002-2026 PCSX2 Dev Team
// SPDX-License-Identifier: GPL-3.0+

#include "PrecompiledHeader.h"
#include "SavedAddressesModel.h"

#include "common/Console.h"

std::map<BreakPointCpu, SavedAddressesModel*> SavedAddressesModel::s_instances;

SavedAddressesModel::SavedAddressesModel(DebugInterface& cpu, QObject* parent)
	: QAbstractTableModel(parent)
	, m_cpu(cpu)
{
}

SavedAddressesModel* SavedAddressesModel::getInstance(DebugInterface& cpu)
{
	auto iterator = s_instances.find(cpu.getCpuType());
	if (iterator == s_instances.end())
		iterator = s_instances.emplace(cpu.getCpuType(), new SavedAddressesModel(cpu)).first;

	return iterator->second;
}

QVariant SavedAddressesModel::data(const QModelIndex& index, int role) const
{
	size_t row = static_cast<size_t>(index.row());
	if (!index.isValid() || row >= m_savedAddresses.size())
		return false;

	const SavedAddress& entry = m_savedAddresses[row];

	if (role == Qt::CheckStateRole)
	{
		if (index.column() == HeaderColumns::LOCKED)
			return entry.locked ? Qt::Checked : Qt::Unchecked;
		return QVariant();
	}

	if (role == Qt::DisplayRole || role == Qt::EditRole)
	{
		switch (index.column())
		{
			case HeaderColumns::ADDRESS:
				return QString::number(entry.address, 16).toUpper();
			case HeaderColumns::LABEL:
				return entry.label;
			case HeaderColumns::DESCRIPTION:
				return entry.description;
			case HeaderColumns::SIZE:
				return QString::number(entry.size);
			case HeaderColumns::LOCKED_VALUE:
				return QString::number(entry.locked_value, 16).toUpper();
			case HeaderColumns::LOCKED:
				return entry.locked ? QString("1") : QString("0");
		}
	}
	if (role == Qt::UserRole)
	{
		switch (index.column())
		{
			case HeaderColumns::ADDRESS:
				return entry.address;
			case HeaderColumns::LABEL:
				return entry.label;
			case HeaderColumns::DESCRIPTION:
				return entry.description;
			case HeaderColumns::SIZE:
				return entry.size;
			case HeaderColumns::LOCKED_VALUE:
				return entry.locked_value;
			case HeaderColumns::LOCKED:
				return entry.locked;
		}
	}

	return QVariant();
}

bool SavedAddressesModel::setData(const QModelIndex& index, const QVariant& value, int role)
{
	size_t row = static_cast<size_t>(index.row());
	if (!index.isValid() || row >= m_savedAddresses.size())
		return false;

	SavedAddress& entry = m_savedAddresses[row];

	if (role == Qt::CheckStateRole)
	{
		if (index.column() == HeaderColumns::LOCKED)
		{
			entry.locked = (value.toInt() == Qt::Checked);
			emit dataChanged(index, index, QList<int>(role));
			return true;
		}
		return false;
	}

	if (role == Qt::EditRole)
	{
		if (index.column() == HeaderColumns::ADDRESS)
		{
			bool ok = false;
			const u32 address = value.toString().toUInt(&ok, 16);
			if (ok)
				entry.address = address;
			else
				return false;
		}

		if (index.column() == HeaderColumns::DESCRIPTION)
			entry.description = value.toString();

		if (index.column() == HeaderColumns::LABEL)
			entry.label = value.toString();

		if (index.column() == HeaderColumns::SIZE)
		{
			bool ok = false;
			const u32 size = value.toString().toUInt(&ok);
			if (ok && (size == 1 || size == 2 || size == 4 || size == 8))
				entry.size = size;
			else
				return false;
		}

		if (index.column() == HeaderColumns::LOCKED_VALUE)
		{
			bool ok = false;
			const u64 locked_value = value.toString().toULongLong(&ok, 16);
			if (ok)
				entry.locked_value = locked_value;
			else
				return false;
		}

		emit dataChanged(index, index, QList<int>(role));
		return true;
	}
	else if (role == Qt::UserRole)
	{
		if (index.column() == HeaderColumns::ADDRESS)
		{
			const u32 address = value.toUInt();
			entry.address = address;
		}

		if (index.column() == HeaderColumns::DESCRIPTION)
			entry.description = value.toString();

		if (index.column() == HeaderColumns::LABEL)
			entry.label = value.toString();

		if (index.column() == HeaderColumns::SIZE)
			entry.size = value.toUInt();

		if (index.column() == HeaderColumns::LOCKED_VALUE)
			entry.locked_value = value.toULongLong();

		if (index.column() == HeaderColumns::LOCKED)
			entry.locked = value.toBool();

		emit dataChanged(index, index, QList<int>(role));
		return true;
	}

	return false;
}

QVariant SavedAddressesModel::headerData(int section, Qt::Orientation orientation, int role) const
{
	if (orientation != Qt::Horizontal)
		return QVariant();

	if (role == Qt::DisplayRole)
	{
		switch (section)
		{
			case SavedAddressesModel::ADDRESS:
				return tr("MEMORY ADDRESS");
			case SavedAddressesModel::LABEL:
				return tr("LABEL");
			case SavedAddressesModel::DESCRIPTION:
				return tr("DESCRIPTION");
			case SavedAddressesModel::SIZE:
				return tr("SIZE");
			case SavedAddressesModel::LOCKED_VALUE:
				return tr("LOCKED VALUE");
			case SavedAddressesModel::LOCKED:
				return tr("LOCKED");
			default:
				return QVariant();
		}
	}
	if (role == Qt::UserRole)
	{
		switch (section)
		{
			case SavedAddressesModel::ADDRESS:
				return "MEMORY ADDRESS";
			case SavedAddressesModel::LABEL:
				return "LABEL";
			case SavedAddressesModel::DESCRIPTION:
				return "DESCRIPTION";
			case SavedAddressesModel::SIZE:
				return "SIZE";
			case SavedAddressesModel::LOCKED_VALUE:
				return "LOCKED VALUE";
			case SavedAddressesModel::LOCKED:
				return "LOCKED";
			default:
				return QVariant();
		}
	}
	return QVariant();
}

Qt::ItemFlags SavedAddressesModel::flags(const QModelIndex& index) const
{
	if (index.column() == HeaderColumns::LOCKED)
		return Qt::ItemIsEnabled | Qt::ItemIsSelectable | Qt::ItemIsUserCheckable;
	return Qt::ItemIsEnabled | Qt::ItemIsSelectable | Qt::ItemIsEditable;
}

void SavedAddressesModel::addRow()
{
	const SavedAddress defaultNewAddress = {0, tr("Name"), tr("Description"), false, 0, 4};
	addRow(defaultNewAddress);
}

void SavedAddressesModel::addRow(SavedAddress addresstoSave)
{
	const int newRowIndex = m_savedAddresses.size();
	beginInsertRows(QModelIndex(), newRowIndex, newRowIndex);
	m_savedAddresses.push_back(addresstoSave);
	endInsertRows();
}

bool SavedAddressesModel::removeRows(int row, int count, const QModelIndex& parent)
{
	if (row < 0 || count < 1 || static_cast<size_t>(row + count) > m_savedAddresses.size())
		return false;

	beginRemoveRows(parent, row, row + count - 1);
	m_savedAddresses.erase(m_savedAddresses.begin() + row, m_savedAddresses.begin() + row + count);
	endRemoveRows();
	return true;
}

int SavedAddressesModel::rowCount(const QModelIndex&) const
{
	return m_savedAddresses.size();
}

int SavedAddressesModel::columnCount(const QModelIndex&) const
{
	return HeaderColumns::COLUMN_COUNT;
}

void SavedAddressesModel::loadSavedAddressFromFieldList(QStringList fields)
{
	if (fields.size() != SavedAddressesModel::HeaderColumns::COLUMN_COUNT && fields.size() != 3)
	{
		Console.WriteLn("Debugger Saved Addresses Model: Invalid number of columns, skipping");
		return;
	}

	bool ok;
	const u32 address = fields[SavedAddressesModel::HeaderColumns::ADDRESS].toUInt(&ok, 16);
	if (!ok)
	{
		Console.WriteLn("Debugger Saved Addresses Model: Failed to parse address '%s', skipping", fields[SavedAddressesModel::HeaderColumns::ADDRESS].toUtf8().constData());
		return;
	}

	const QString label = fields[SavedAddressesModel::HeaderColumns::LABEL];
	const QString description = fields[SavedAddressesModel::HeaderColumns::DESCRIPTION];

	SavedAddressesModel::SavedAddress importedAddress = {address, label, description, false, 0, 4};

	if (fields.size() == SavedAddressesModel::HeaderColumns::COLUMN_COUNT)
	{
		u32 size = fields[SavedAddressesModel::HeaderColumns::SIZE].toUInt(&ok);
		if (ok && (size == 1 || size == 2 || size == 4 || size == 8))
			importedAddress.size = size;

		u64 locked_value = fields[SavedAddressesModel::HeaderColumns::LOCKED_VALUE].toULongLong(&ok, 16);
		if (ok)
			importedAddress.locked_value = locked_value;

		importedAddress.locked = (fields[SavedAddressesModel::HeaderColumns::LOCKED].toInt() == 1);
	}

	addRow(importedAddress);
}

void SavedAddressesModel::clear()
{
	beginResetModel();
	m_savedAddresses.clear();
	endResetModel();
}

#include "moc_SavedAddressesModel.cpp"
