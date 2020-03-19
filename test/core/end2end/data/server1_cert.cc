/*
 *
 * Copyright 2015 gRPC authors.
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
 *
 */

extern const char test_server1_cert[] = {
    0x2d, 0x2d, 0x2d, 0x2d, 0x2d, 0x42, 0x45, 0x47, 0x49, 0x4e, 0x20, 0x43,
    0x45, 0x52, 0x54, 0x49, 0x46, 0x49, 0x43, 0x41, 0x54, 0x45, 0x2d, 0x2d,
    0x2d, 0x2d, 0x2d, 0x0a, 0x4d, 0x49, 0x49, 0x44, 0x74, 0x44, 0x43, 0x43,
    0x41, 0x70, 0x79, 0x67, 0x41, 0x77, 0x49, 0x42, 0x41, 0x67, 0x49, 0x55,
    0x62, 0x4a, 0x66, 0x54, 0x52, 0x45, 0x4a, 0x36, 0x6b, 0x36, 0x2f, 0x2b,
    0x6f, 0x49, 0x6e, 0x57, 0x68, 0x56, 0x31, 0x4f, 0x31, 0x6a, 0x33, 0x5a,
    0x54, 0x30, 0x49, 0x77, 0x44, 0x51, 0x59, 0x4a, 0x4b, 0x6f, 0x5a, 0x49,
    0x68, 0x76, 0x63, 0x4e, 0x41, 0x51, 0x45, 0x4c, 0x0a, 0x42, 0x51, 0x41,
    0x77, 0x56, 0x6a, 0x45, 0x4c, 0x4d, 0x41, 0x6b, 0x47, 0x41, 0x31, 0x55,
    0x45, 0x42, 0x68, 0x4d, 0x43, 0x51, 0x56, 0x55, 0x78, 0x45, 0x7a, 0x41,
    0x52, 0x42, 0x67, 0x4e, 0x56, 0x42, 0x41, 0x67, 0x4d, 0x43, 0x6c, 0x4e,
    0x76, 0x62, 0x57, 0x55, 0x74, 0x55, 0x33, 0x52, 0x68, 0x64, 0x47, 0x55,
    0x78, 0x49, 0x54, 0x41, 0x66, 0x42, 0x67, 0x4e, 0x56, 0x42, 0x41, 0x6f,
    0x4d, 0x0a, 0x47, 0x45, 0x6c, 0x75, 0x64, 0x47, 0x56, 0x79, 0x62, 0x6d,
    0x56, 0x30, 0x49, 0x46, 0x64, 0x70, 0x5a, 0x47, 0x64, 0x70, 0x64, 0x48,
    0x4d, 0x67, 0x55, 0x48, 0x52, 0x35, 0x49, 0x45, 0x78, 0x30, 0x5a, 0x44,
    0x45, 0x50, 0x4d, 0x41, 0x30, 0x47, 0x41, 0x31, 0x55, 0x45, 0x41, 0x77,
    0x77, 0x47, 0x64, 0x47, 0x56, 0x7a, 0x64, 0x47, 0x4e, 0x68, 0x4d, 0x42,
    0x34, 0x58, 0x44, 0x54, 0x49, 0x77, 0x0a, 0x4d, 0x44, 0x4d, 0x78, 0x4f,
    0x44, 0x41, 0x7a, 0x4d, 0x54, 0x41, 0x30, 0x4d, 0x6c, 0x6f, 0x58, 0x44,
    0x54, 0x4d, 0x77, 0x4d, 0x44, 0x4d, 0x78, 0x4e, 0x6a, 0x41, 0x7a, 0x4d,
    0x54, 0x41, 0x30, 0x4d, 0x6c, 0x6f, 0x77, 0x5a, 0x54, 0x45, 0x4c, 0x4d,
    0x41, 0x6b, 0x47, 0x41, 0x31, 0x55, 0x45, 0x42, 0x68, 0x4d, 0x43, 0x56,
    0x56, 0x4d, 0x78, 0x45, 0x54, 0x41, 0x50, 0x42, 0x67, 0x4e, 0x56, 0x0a,
    0x42, 0x41, 0x67, 0x4d, 0x43, 0x45, 0x6c, 0x73, 0x62, 0x47, 0x6c, 0x75,
    0x62, 0x32, 0x6c, 0x7a, 0x4d, 0x52, 0x41, 0x77, 0x44, 0x67, 0x59, 0x44,
    0x56, 0x51, 0x51, 0x48, 0x44, 0x41, 0x64, 0x44, 0x61, 0x47, 0x6c, 0x6a,
    0x59, 0x57, 0x64, 0x76, 0x4d, 0x52, 0x55, 0x77, 0x45, 0x77, 0x59, 0x44,
    0x56, 0x51, 0x51, 0x4b, 0x44, 0x41, 0x78, 0x46, 0x65, 0x47, 0x46, 0x74,
    0x63, 0x47, 0x78, 0x6c, 0x0a, 0x4c, 0x43, 0x42, 0x44, 0x62, 0x79, 0x34,
    0x78, 0x47, 0x6a, 0x41, 0x59, 0x42, 0x67, 0x4e, 0x56, 0x42, 0x41, 0x4d,
    0x4d, 0x45, 0x53, 0x6f, 0x75, 0x64, 0x47, 0x56, 0x7a, 0x64, 0x43, 0x35,
    0x6e, 0x62, 0x32, 0x39, 0x6e, 0x62, 0x47, 0x55, 0x75, 0x59, 0x32, 0x39,
    0x74, 0x4d, 0x49, 0x49, 0x42, 0x49, 0x6a, 0x41, 0x4e, 0x42, 0x67, 0x6b,
    0x71, 0x68, 0x6b, 0x69, 0x47, 0x39, 0x77, 0x30, 0x42, 0x0a, 0x41, 0x51,
    0x45, 0x46, 0x41, 0x41, 0x4f, 0x43, 0x41, 0x51, 0x38, 0x41, 0x4d, 0x49,
    0x49, 0x42, 0x43, 0x67, 0x4b, 0x43, 0x41, 0x51, 0x45, 0x41, 0x35, 0x78,
    0x4f, 0x4f, 0x4e, 0x78, 0x4a, 0x4a, 0x38, 0x62, 0x38, 0x51, 0x61, 0x75,
    0x76, 0x6f, 0x62, 0x35, 0x2f, 0x37, 0x64, 0x50, 0x59, 0x5a, 0x66, 0x49,
    0x63, 0x64, 0x2b, 0x75, 0x68, 0x41, 0x57, 0x4c, 0x32, 0x5a, 0x6c, 0x54,
    0x50, 0x7a, 0x0a, 0x51, 0x76, 0x75, 0x34, 0x6f, 0x46, 0x30, 0x51, 0x49,
    0x34, 0x69, 0x59, 0x67, 0x50, 0x35, 0x69, 0x47, 0x67, 0x72, 0x79, 0x39,
    0x7a, 0x45, 0x74, 0x43, 0x4d, 0x2b, 0x59, 0x51, 0x53, 0x38, 0x55, 0x68,
    0x69, 0x41, 0x6c, 0x50, 0x6c, 0x71, 0x61, 0x36, 0x41, 0x4e, 0x78, 0x67,
    0x69, 0x42, 0x53, 0x45, 0x79, 0x4d, 0x48, 0x48, 0x2f, 0x78, 0x45, 0x38,
    0x6c, 0x6f, 0x2f, 0x2b, 0x63, 0x61, 0x59, 0x0a, 0x47, 0x65, 0x41, 0x43,
    0x71, 0x79, 0x36, 0x34, 0x30, 0x4a, 0x70, 0x6c, 0x2f, 0x4a, 0x6f, 0x63,
    0x46, 0x47, 0x6f, 0x33, 0x78, 0x64, 0x31, 0x4c, 0x38, 0x44, 0x43, 0x61,
    0x77, 0x6a, 0x6c, 0x61, 0x6a, 0x36, 0x65, 0x75, 0x37, 0x54, 0x37, 0x54,
    0x2f, 0x74, 0x70, 0x41, 0x56, 0x32, 0x71, 0x71, 0x31, 0x33, 0x62, 0x35,
    0x37, 0x31, 0x30, 0x65, 0x4e, 0x52, 0x62, 0x43, 0x41, 0x66, 0x46, 0x65,
    0x0a, 0x38, 0x79, 0x41, 0x4c, 0x69, 0x47, 0x51, 0x65, 0x6d, 0x78, 0x30,
    0x49, 0x59, 0x68, 0x6c, 0x5a, 0x58, 0x4e, 0x62, 0x49, 0x47, 0x57, 0x4c,
    0x42, 0x4e, 0x68, 0x42, 0x68, 0x76, 0x56, 0x6a, 0x4a, 0x68, 0x37, 0x55,
    0x76, 0x4f, 0x71, 0x70, 0x41, 0x44, 0x6b, 0x34, 0x78, 0x74, 0x6c, 0x38,
    0x6f, 0x35, 0x6a, 0x30, 0x78, 0x67, 0x4d, 0x49, 0x52, 0x67, 0x36, 0x57,
    0x4a, 0x47, 0x4b, 0x36, 0x63, 0x0a, 0x36, 0x66, 0x66, 0x53, 0x49, 0x67,
    0x34, 0x65, 0x50, 0x31, 0x58, 0x6d, 0x6f, 0x76, 0x4e, 0x59, 0x5a, 0x39,
    0x4c, 0x4c, 0x45, 0x4a, 0x47, 0x36, 0x38, 0x74, 0x46, 0x30, 0x51, 0x2f,
    0x79, 0x49, 0x4e, 0x34, 0x33, 0x42, 0x34, 0x64, 0x74, 0x31, 0x6f, 0x71,
    0x34, 0x6a, 0x7a, 0x53, 0x64, 0x43, 0x62, 0x47, 0x34, 0x46, 0x31, 0x45,
    0x69, 0x79, 0x6b, 0x54, 0x32, 0x54, 0x6d, 0x77, 0x50, 0x56, 0x0a, 0x59,
    0x44, 0x69, 0x38, 0x74, 0x6d, 0x6c, 0x36, 0x44, 0x66, 0x4f, 0x43, 0x44,
    0x47, 0x6e, 0x69, 0x74, 0x38, 0x73, 0x76, 0x6e, 0x4d, 0x45, 0x6d, 0x42,
    0x76, 0x2f, 0x66, 0x63, 0x50, 0x64, 0x33, 0x31, 0x47, 0x53, 0x62, 0x58,
    0x6a, 0x46, 0x38, 0x4d, 0x2b, 0x4b, 0x47, 0x47, 0x51, 0x49, 0x44, 0x41,
    0x51, 0x41, 0x42, 0x6f, 0x32, 0x73, 0x77, 0x61, 0x54, 0x41, 0x4a, 0x42,
    0x67, 0x4e, 0x56, 0x0a, 0x48, 0x52, 0x4d, 0x45, 0x41, 0x6a, 0x41, 0x41,
    0x4d, 0x41, 0x73, 0x47, 0x41, 0x31, 0x55, 0x64, 0x44, 0x77, 0x51, 0x45,
    0x41, 0x77, 0x49, 0x46, 0x34, 0x44, 0x42, 0x50, 0x42, 0x67, 0x4e, 0x56,
    0x48, 0x52, 0x45, 0x45, 0x53, 0x44, 0x42, 0x47, 0x67, 0x68, 0x41, 0x71,
    0x4c, 0x6e, 0x52, 0x6c, 0x63, 0x33, 0x51, 0x75, 0x5a, 0x32, 0x39, 0x76,
    0x5a, 0x32, 0x78, 0x6c, 0x4c, 0x6d, 0x5a, 0x79, 0x0a, 0x67, 0x68, 0x68,
    0x33, 0x59, 0x58, 0x52, 0x6c, 0x63, 0x6e, 0x70, 0x76, 0x62, 0x32, 0x6b,
    0x75, 0x64, 0x47, 0x56, 0x7a, 0x64, 0x43, 0x35, 0x6e, 0x62, 0x32, 0x39,
    0x6e, 0x62, 0x47, 0x55, 0x75, 0x59, 0x6d, 0x57, 0x43, 0x45, 0x69, 0x6f,
    0x75, 0x64, 0x47, 0x56, 0x7a, 0x64, 0x43, 0x35, 0x35, 0x62, 0x33, 0x56,
    0x30, 0x64, 0x57, 0x4a, 0x6c, 0x4c, 0x6d, 0x4e, 0x76, 0x62, 0x59, 0x63,
    0x45, 0x0a, 0x77, 0x4b, 0x67, 0x42, 0x41, 0x7a, 0x41, 0x4e, 0x42, 0x67,
    0x6b, 0x71, 0x68, 0x6b, 0x69, 0x47, 0x39, 0x77, 0x30, 0x42, 0x41, 0x51,
    0x73, 0x46, 0x41, 0x41, 0x4f, 0x43, 0x41, 0x51, 0x45, 0x41, 0x53, 0x38,
    0x68, 0x44, 0x51, 0x41, 0x38, 0x50, 0x53, 0x67, 0x69, 0x70, 0x67, 0x41,
    0x6d, 0x6c, 0x37, 0x51, 0x33, 0x2f, 0x64, 0x6a, 0x77, 0x51, 0x36, 0x34,
    0x34, 0x67, 0x68, 0x57, 0x51, 0x76, 0x0a, 0x43, 0x32, 0x4b, 0x62, 0x2b,
    0x72, 0x33, 0x30, 0x52, 0x43, 0x59, 0x31, 0x45, 0x79, 0x4b, 0x4e, 0x68,
    0x6e, 0x51, 0x6e, 0x49, 0x49, 0x68, 0x2f, 0x4f, 0x55, 0x62, 0x42, 0x5a,
    0x76, 0x68, 0x30, 0x4d, 0x30, 0x69, 0x59, 0x73, 0x79, 0x36, 0x78, 0x71,
    0x58, 0x67, 0x66, 0x44, 0x68, 0x43, 0x42, 0x39, 0x33, 0x41, 0x41, 0x36,
    0x6a, 0x30, 0x69, 0x35, 0x63, 0x53, 0x38, 0x66, 0x6b, 0x68, 0x48, 0x0a,
    0x4a, 0x6c, 0x34, 0x52, 0x4b, 0x30, 0x74, 0x53, 0x6b, 0x47, 0x51, 0x33,
    0x59, 0x4e, 0x59, 0x34, 0x4e, 0x7a, 0x58, 0x77, 0x51, 0x50, 0x2f, 0x76,
    0x6d, 0x55, 0x67, 0x66, 0x6b, 0x77, 0x38, 0x56, 0x42, 0x41, 0x5a, 0x34,
    0x59, 0x34, 0x47, 0x4b, 0x78, 0x70, 0x70, 0x64, 0x41, 0x54, 0x6a, 0x66,
    0x66, 0x49, 0x57, 0x2b, 0x73, 0x72, 0x62, 0x41, 0x6d, 0x64, 0x44, 0x72,
    0x75, 0x49, 0x52, 0x4d, 0x0a, 0x77, 0x50, 0x65, 0x69, 0x6b, 0x67, 0x4f,
    0x6f, 0x52, 0x72, 0x58, 0x66, 0x30, 0x4c, 0x41, 0x31, 0x66, 0x69, 0x34,
    0x54, 0x71, 0x78, 0x41, 0x52, 0x7a, 0x65, 0x52, 0x77, 0x65, 0x6e, 0x51,
    0x70, 0x61, 0x79, 0x4e, 0x66, 0x47, 0x48, 0x54, 0x76, 0x56, 0x46, 0x39,
    0x61, 0x4a, 0x6b, 0x6c, 0x38, 0x48, 0x6f, 0x61, 0x4d, 0x75, 0x6e, 0x54,
    0x41, 0x64, 0x47, 0x35, 0x70, 0x49, 0x56, 0x63, 0x72, 0x0a, 0x39, 0x47,
    0x4b, 0x69, 0x2f, 0x67, 0x45, 0x4d, 0x70, 0x58, 0x55, 0x4a, 0x62, 0x62,
    0x56, 0x76, 0x33, 0x55, 0x35, 0x66, 0x72, 0x58, 0x31, 0x57, 0x6f, 0x34,
    0x43, 0x46, 0x6f, 0x2b, 0x72, 0x5a, 0x57, 0x4a, 0x2f, 0x4c, 0x79, 0x43,
    0x4d, 0x65, 0x62, 0x30, 0x6a, 0x63, 0x69, 0x4e, 0x4c, 0x78, 0x53, 0x64,
    0x4d, 0x77, 0x6a, 0x2f, 0x45, 0x2f, 0x5a, 0x75, 0x45, 0x78, 0x6c, 0x79,
    0x65, 0x5a, 0x0a, 0x67, 0x63, 0x39, 0x63, 0x74, 0x50, 0x6a, 0x53, 0x4d,
    0x76, 0x67, 0x53, 0x79, 0x58, 0x45, 0x4b, 0x76, 0x36, 0x56, 0x77, 0x6f,
    0x62, 0x6c, 0x65, 0x65, 0x67, 0x38, 0x38, 0x56, 0x32, 0x5a, 0x67, 0x7a,
    0x65, 0x6e, 0x7a, 0x69, 0x4f, 0x52, 0x6f, 0x57, 0x6a, 0x34, 0x4b, 0x73,
    0x7a, 0x47, 0x2f, 0x6c, 0x62, 0x51, 0x5a, 0x76, 0x67, 0x3d, 0x3d, 0x0a,
    0x2d, 0x2d, 0x2d, 0x2d, 0x2d, 0x45, 0x4e, 0x44, 0x20, 0x43, 0x45, 0x52,
    0x54, 0x49, 0x46, 0x49, 0x43, 0x41, 0x54, 0x45, 0x2d, 0x2d, 0x2d, 0x2d,
    0x2d, 0x0a};
