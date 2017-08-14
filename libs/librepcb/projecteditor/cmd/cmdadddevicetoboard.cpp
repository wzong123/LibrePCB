/*
 * LibrePCB - Professional EDA for everyone!
 * Copyright (C) 2013 LibrePCB Developers, see AUTHORS.md for contributors.
 * http://librepcb.org/
 *
 * This program is free software: you can redistribute it and/or modify
 * it under the terms of the GNU General Public License as published by
 * the Free Software Foundation, either version 3 of the License, or
 * (at your option) any later version.
 *
 * This program is distributed in the hope that it will be useful,
 * but WITHOUT ANY WARRANTY; without even the implied warranty of
 * MERCHANTABILITY or FITNESS FOR A PARTICULAR PURPOSE.  See the
 * GNU General Public License for more details.
 *
 * You should have received a copy of the GNU General Public License
 * along with this program.  If not, see <http://www.gnu.org/licenses/>.
 */

/*****************************************************************************************
 *  Includes
 ****************************************************************************************/
#include <QtCore>
#include "cmdadddevicetoboard.h"
#include <librepcb/workspace/library/workspacelibrarydb.h>
#include <librepcb/library/cmp/component.h>
#include <librepcb/library/dev/device.h>
#include <librepcb/library/pkg/package.h>
#include <librepcb/workspace/workspace.h>
#include <librepcb/project/project.h>
#include <librepcb/project/library/projectlibrary.h>
#include <librepcb/project/boards/board.h>
#include <librepcb/project/circuit/componentinstance.h>
#include <librepcb/project/library/cmd/cmdprojectlibraryaddelement.h>
#include <librepcb/project/boards/cmd/cmddeviceinstanceadd.h>

/*****************************************************************************************
 *  Namespace
 ****************************************************************************************/
namespace librepcb {
namespace project {
namespace editor {

/*****************************************************************************************
 *  Constructors / Destructor
 ****************************************************************************************/

CmdAddDeviceToBoard::CmdAddDeviceToBoard(workspace::Workspace& workspace, Board& board,
        ComponentInstance& cmpInstance, const Uuid& deviceUuid, const Uuid& footprintUuid,
        const Point& position, const Angle& rotation, bool mirror) noexcept :
    UndoCommandGroup(tr("Add device to board")),
    mWorkspace(workspace), mBoard(board), mComponentInstance(cmpInstance),
    mDeviceUuid(deviceUuid), mFootprintUuid(footprintUuid), mPosition(position),
    mRotation(rotation), mMirror(mirror),
    mCmdAddToBoard(nullptr)
{
}

CmdAddDeviceToBoard::~CmdAddDeviceToBoard() noexcept
{
}

/*****************************************************************************************
 *  Getters
 ****************************************************************************************/

BI_Device* CmdAddDeviceToBoard::getDeviceInstance() const noexcept
{
    Q_ASSERT(mCmdAddToBoard);
    return mCmdAddToBoard ? mCmdAddToBoard->getDeviceInstance() : nullptr;
}

/*****************************************************************************************
 *  Inherited from UndoCommand
 ****************************************************************************************/

bool CmdAddDeviceToBoard::performExecute()
{
    // if there is no such device in the project's library, copy it from the
    // workspace library to the project's library
    library::Device* dev = mBoard.getProject().getLibrary().getDevice(mDeviceUuid);
    if (!dev) {
        FilePath devFp = mWorkspace.getLibraryDb().getLatestDevice(mDeviceUuid);
        if (!devFp.isValid()) {
            throw RuntimeError(__FILE__, __LINE__,
                QString(tr("The device with the UUID \"%1\" does not exist in the "
                "workspace library!")).arg(mDeviceUuid.toStr()));
        }
        dev = new library::Device(devFp, true);
        CmdProjectLibraryAddElement<library::Device>* cmdAddToLibrary =
            new CmdProjectLibraryAddElement<library::Device>(mBoard.getProject().getLibrary(), *dev);
        appendChild(cmdAddToLibrary); // can throw
    }
    Q_ASSERT(dev);

    // if there is no such package in the project's library, copy it from the
    // workspace library to the project's library
    Uuid pkgUuid = dev->getPackageUuid();
    library::Package* pkg = mBoard.getProject().getLibrary().getPackage(pkgUuid);
    if (!pkg) {
        FilePath pkgFp = mWorkspace.getLibraryDb().getLatestPackage(pkgUuid);
        if (!pkgFp.isValid()) {
            throw RuntimeError(__FILE__, __LINE__,
                QString(tr("The package with the UUID \"%1\" does not exist in the "
                "workspace library!")).arg(pkgUuid.toStr()));
        }
        pkg = new library::Package(pkgFp, true);
        CmdProjectLibraryAddElement<library::Package>* cmdAddToLibrary =
            new CmdProjectLibraryAddElement<library::Package>(mBoard.getProject().getLibrary(), *pkg);
        appendChild(cmdAddToLibrary); // can throw
    }
    Q_ASSERT(pkg);

    // TODO: remove this!
    if (mFootprintUuid.isNull() && (!pkg->getFootprintUuids().isEmpty())) {
        mFootprintUuid = pkg->getFootprintUuids().first();
    }

    // create child command to add a new device instance to the board
    mCmdAddToBoard = new CmdDeviceInstanceAdd(mBoard, mComponentInstance, mDeviceUuid,
                                              mFootprintUuid, mPosition, mRotation, mMirror);
    appendChild(mCmdAddToBoard); // can throw

    // execute all child commands
    return UndoCommandGroup::performExecute(); // can throw
}

/*****************************************************************************************
 *  End of File
 ****************************************************************************************/

} // namespace editor
} // namespace project
} // namespace librepcb
