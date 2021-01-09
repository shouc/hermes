/*
 * Copyright (c) Facebook, Inc. and its affiliates.
 *
 * This source code is licensed under the MIT license found in the
 * LICENSE file in the root directory of this source tree.
 */

#include <hermes/hermes.h>
#include <jsi/jsi.h>
#include <cstring>
#include <fstream>
#include <sstream>
using facebook::hermes::HermesRuntime;
using facebook::hermes::makeHermesRuntime;
using facebook::jsi::HostObject;
using facebook::jsi::JSIException;
using facebook::jsi::Object;
using facebook::jsi::PropNameID;
using facebook::jsi::Runtime;
using facebook::jsi::String;
using facebook::jsi::StringBuffer;
using facebook::jsi::Value;

int main(int argc, char** argv){
  auto runtime = makeHermesRuntime();
  std::string line;
  std::ifstream infile(argv[1]);
  while (std::getline(infile, line))
  {
    try {
      runtime->evaluateJavaScript(std::make_unique<StringBuffer>(line), "");
    } catch (const JSIException &e) {
//      printf("failed\n");
    }
    // process pair (a,b)
  }
  return 0;
}

