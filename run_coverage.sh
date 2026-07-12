#!/bin/bash
set -e

echo "Checking prerequisites..."

if ! command -v lcov &> /dev/null; then
    echo "lcov not found. Installing via Homebrew..."
    brew install lcov
else
    echo "lcov is installed."
fi

if ! command -v genhtml &> /dev/null; then
    echo "genhtml not found. Installing via Homebrew (part of lcov)..."
    brew install lcov
else
    echo "genhtml is installed."
fi

if ! command -v node &> /dev/null; then
    echo "Node.js not found. Please install Node.js."
    exit 1
else
    echo "Node.js is installed."
fi

if ! command -v emcmake &> /dev/null; then
    echo "emcmake not found. Please ensure Emscripten is installed and activated."
    exit 1
else
    echo "Emscripten is active."
fi

echo "All prerequisites met."
echo "Building and running tests with coverage..."

npm run test:coverage

echo "Coverage report generated successfully."
echo "Starting coverage server..."

npm run coverage
