/*
 * Copyright (c) 2018-present, Trail of Bits, Inc.
 *
 * Licensed under the Apache License, Version 2.0 (the "License");
 * you may not use this file except in compliance with the License.
 * You may obtain a copy of the License at
 *
 *     http://www.apache.org/licenses/LICENSE-2.0
 *
 * Unless required by applicable law or agreed to in writing, software
 * distributed under the License is distributed on an "AS IS" BASIS,
 * WITHOUT WARRANTIES OR CONDITIONS OF ANY KIND, either express or implied.
 * See the License for the specific language governing permissions and
 * limitations under the License.
 */

#pragma once

#include "cmdline.h"
#include "istatus.h"
#include "types.h"

/// Error code returned by ABILibGeneratorStatus objects
enum class ABILibGeneratorError { IOError, Unknown };

/// Status object used by the generateABILibrary function
using ABILibGeneratorStatus = IStatus<ABILibGeneratorError>;

/// Generates the ABI library using the provided command line options with the
/// given ABI library state
ABILibGeneratorStatus generateABILibrary(
    const CommandLineOptions &cmdline_options, const ABILibrary &abi_library,
    const Profile &profile);
