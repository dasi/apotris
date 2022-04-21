#include "tetramino.hpp"
#include "tetrisgame.h"
#include <stdlib.h>
#include <iostream>
#include "tonc_core.h"


using namespace Tetris;

int Game::checkRotation(int dx, int dy, int r) {
    int x = dx + pawn.x;
    int y = dy + pawn.y;

    int total = 0;
    for (int i = 0; i < 4; i++) {
        if(total >= 4)
            break;

        for (int j = 0; j < 4; j++) {
            if (pawn.board[r][i][j] == 0)
                continue;
            
            total++;

            if (x + j < 0 || x + j > lengthX - 1 || y + i < 0 || y + i > lengthY - 1 || board[i + y][j + x] != 0) 
                return 0;
            
            if(total >= 4)
                break;
        }
    }
    return 1;
}

void Pawn::setBlock() {
    for (int i = 0; i < 4; i++)
        for (int j = 0; j < 4; j++)
            for (int k = 0; k < 4; k++)
                Pawn::board[i][j][k] = GameInfo::tetraminos[current][i][j][k];
}

void Game::rotateCW() {
    if (clearLock)
        return;
    lastMoveRotation = 1;
    int len = (pawn.current == 0) ? 7 : 6;
    for (int i = 0; i < len; i++) {

        if(i == 1 || (len == 7 && i == 2))
            continue; 
        int dx = GameInfo::kicks[(pawn.current == 0)][0][pawn.rotation][i][0];
        int dy = GameInfo::kicks[(pawn.current == 0)][0][pawn.rotation][i][1];
        int r = (pawn.rotation == 3) ? 0 : pawn.rotation + 1;
        if (checkRotation(dx, dy, r)) {
            pawn.rotation++;
            if (pawn.rotation > 3)
                pawn.rotation = 0;

            pawn.x += dx;
            pawn.y += dy;

            lockCheck();

            pawn.setBlock();
            sounds.rotate = 1;
            return;
        }else
			sounds.invalid = 1;
    }
}

void Game::rotateCCW() {
    if (clearLock)
        return;
    lastMoveRotation = 1;
    int len = (pawn.current == 0) ? 7 : 6;
    for (int i = 0; i < len; i++) {
        if(i == 1 || (len == 7 && i == 2))
            continue; 
        int dx = GameInfo::kicks[(pawn.current == 0)][1][pawn.rotation][i][0];
        int dy = GameInfo::kicks[(pawn.current == 0)][1][pawn.rotation][i][1];
        int r = (pawn.rotation == 0) ? 3 : pawn.rotation - 1;
        if (checkRotation(dx, dy, r)) {
            pawn.rotation--;
            if (pawn.rotation < 0)
                pawn.rotation = 3;

            pawn.x += dx;
            pawn.y += dy;

            lockCheck();

            pawn.setBlock();
            sounds.rotate = 1;
            return;
        }else
			sounds.invalid = 1;
    }
}

void Game::moveLeft() {
    if (checkRotation(-1, 0, pawn.rotation)) {
        pawn.x--;
        sounds.shift = 1;
        lastMoveRotation = 0;
        lockCheck();
    }else{
        pushDir = -1;
    }
}

void Game::moveRight() {
    if (checkRotation(1, 0, pawn.rotation)) {
        pawn.x++;
        sounds.shift = 1;
        lastMoveRotation = 0;
        lockCheck();
    }else{
        pushDir = 1;
    }
}

void Game::moveDown() {
    if (checkRotation(0, 1, pawn.rotation))
        pawn.y++;
}

Drop Game::hardDrop() {

    Drop result;

    if (clearLock){
        result.startX = -1;
        return result;
    }

    int diff = pawn.lowest - pawn.y;

    result.on = true;
    result.startX = pawn.x;

    int start = -1;
    int end = 4;
    for(int i = 0; i < 4; i++){
        bool found = false;
        for(int j = 0; j < 4; j++){
            if(pawn.board[pawn.rotation][j][i] == 1){
                found = true;
                break;
            }
        }

        if(found && start == -1)
            start = i;
        else if(!found && start != -1 && end == 4)
            end = i;
    }

    result.startX += start;
    result.endX = result.startX + end - start;

    result.startY = pawn.y - 20;

    pawn.y = pawn.lowest;

    int add = 0;
    for(int i = 3; i >=0; i--){
        bool found = false;
        for(int j = 0; j < 4; j++){
            if(pawn.board[pawn.rotation][i][j] == 1){
                found = true;
                break;
            }
        }
        if(found){
            add = i;
            break;
        }
    }
    
    result.endY = pawn.y - 20 + add;

    if((pawn.current == 2 && pawn.rotation == 3) || (pawn.current == 1 && pawn.rotation == 1))
        result.endY--;

    place();
    score += diff * 2;

    return result;
}

void Game::update() {
    if (lost)
        return;
    timer++;
    if (clearLock)
        return;
    int prevLevel = level;
    level = ((int)linesCleared / 10) + 1;
    if(level < prevLevel)
        level = prevLevel;

    if((linesCleared >= goal && gameMode == 1) || (linesCleared >= goal && gameMode == 2) || (garbageCleared >= goal && gameMode == 3))
        won = 1;

    if (prevLevel != level && (gameMode == 0 || gameMode == 2))
        sounds.levelUp = 1;

    if (gameMode == 1 || gameMode == 3)
        speed = GameInfo::speed[0];
    else
        speed = GameInfo::speed[(level < 19) ? level - 1 : 18];

    speedCounter += speed;

    int n = (int)speedCounter;
    for (int i = 0; i < n; i++) {
        moveDown();
    }
    speedCounter -= n;

    pawn.lowest = lowest();

    if(dropping){
        lastDrop = hardDrop();
        dropping = false;
        return;
    }else{
        if (pawn.lowest == pawn.y)
            lockTimer--;
        else if (lockMoveCounter > 0)
            lockTimer = maxLockTimer;

        if (lockTimer == 0)
            place();
    }

    if (!(left || right))
        das = 0;
    else if (das < maxDas)
        das++;

    if (das == maxDas && !(left && right)) {
        arrCounter++;
        if (arrCounter == arr) {
            if (left)
                moveLeft();
            else
                moveRight();
            arrCounter = 0;
        }
    }

    if (down) {
        if (softDropCounter < maxSoftDrop)
            softDropCounter++;
    }
    else
        softDropCounter = 0;

    if (softDropCounter == maxSoftDrop) {
        for (int i = 0; i < softDropSpeed; i++) {
            moveDown();
            if (pawn.y != pawn.lowest)
                score++;
        }
    }
}

int Game::lowest() {

    int blocks[4];

    for(int i = 0; i < 4; i++)//initialize height array
        blocks[i] = -1;

    for(int i = 3; i >= 0; i--)//find height of lowest block in column
        for(int j = 0; j < 4; j++)
            if(pawn.board[pawn.rotation][i][j] && blocks[j] == -1)
                blocks[j] = i;

    for(int i = 0; i < lengthY - pawn.y; i++){
        for(int j = 0; j < 4; j++){
            if(blocks[j] == -1)
                continue;
            int x = pawn.x+j;
            int y = pawn.y+i;
            if(y+blocks[j] >= lengthY || board[y+blocks[j]][x])
                return y-1;
        }
    }
    
    return pawn.y;
}

void Game::place() {
    int i, j;
    for (i = 0; i < 4; i++) {
        for (j = 0; j < 4; j++) {
            if (pawn.board[pawn.rotation][i][j] == 0)
                continue;

            int x = pawn.x + j;
            int y = pawn.y + i;

            if (y > lengthY - 1 || x > lengthX)
                continue;

            board[y][x] = pawn.current + 1;
        }
    }

    if (pawn.y < ((int)lengthY / 2 - 1)) {
        lost = 1;
        return;
    }

    if (clear())
        comboCounter++;
            
    else{
        comboCounter = 0;
        if(gameMode == 3 && garbageHeight < 9){
            generateGarbage(9-garbageHeight);
        }
    }

    if (!clearLock)
        next();

    canHold = 1;
    lockTimer = maxLockTimer;
    lockMoveCounter = 15;
    sounds.place = 1;
    refresh = 1;

    if(finesseCounter > 2)
        finesse++;

    finesseCounter = 0;
    pushDir = 0;
}

int Game::clear() {
    int i, j;
    int clearCount = 0;
    int isTSpin = 0;
    int isPerfectClear = 1;
    int isBackToBack = 0;
    int isDifficult = 0;

    if (pawn.current == 5 && lastMoveRotation) {
        int frontCount = 0;
        int backCount = 0;
        int x = pawn.x;
        int y = pawn.y;

        switch (pawn.rotation) {
        case 0:
            frontCount += (board[y][x] != 0) + (board[y][x + 2] != 0);
            if (y + 2 > lengthY - 1)
                backCount = 2;
            else
                backCount += (board[y + 2][x] != 0) + (board[y + 2][x + 2] != 0);
            break;
        case 1:
            frontCount += (board[y][x + 2] != 0) + (board[y + 2][x + 2] != 0);
            if (x < 0)
                backCount = 2;
            else
                backCount += (board[y][x] != 0) + (board[y + 2][x] != 0);
            break;
        case 2:
            frontCount += (board[y + 2][x] != 0) + (board[y + 2][x + 2] != 0);
            backCount += (board[y][x] != 0) + (board[y][x + 2]);
            break;
        case 3:
            frontCount += (board[y][x] != 0) + (board[y + 2][x] != 0);
            if (x > lengthX - 1)
                backCount = 2;
            else
                backCount += (board[y][x + 2] != 0) + (board[y + 2][x + 2] != 0);
            break;
        }
        if (frontCount == 2 && backCount > 0)
            isTSpin = 2;
        else if (frontCount > 0 && backCount == 2)
            isTSpin = 1;
    }

    int garbageToRemove = 0;

    for (i = 0; i < lengthY; i++) {
        int toClear = 1;
        for (j = 0; j < lengthX; j++)
            if (board[i][j] == 0)
                toClear = 0;

        if (toClear) {
            linesToClear.push_back(i);
            linesCleared++;
            clearCount++;
            if(gameMode == 3 && i >= lengthY-garbageHeight)
                garbageToRemove++;
        }
    }

    garbageHeight-=garbageToRemove;
    garbageCleared+=garbageToRemove;

    if (linesCleared > 0) {
        for (j = 0; j < lengthY; j++) {
            int skip = 0;
            std::list<int>::iterator index = linesToClear.begin();
            for (i = 0; i < (int)linesToClear.size(); i++) {
                if (j == *index) {
                    skip = 1;
                    break;
                }
                std::advance(index, 1);
            }
            if (skip)
                continue;
            for (i = 0; i < lengthX; i++) {
                if (board[j][i]) {
                    isPerfectClear = 0;
                }
            }
        }
    }

    if (clearCount == 0)
        return 0;

    int add = 0;
    switch (isTSpin) {
    case 0:
        add += GameInfo::scoring[clearCount - 1][0] * level;
        isDifficult = GameInfo::scoring[clearCount - 1][1];
        break;
    case 1:
        add += GameInfo::scoring[clearCount - 1 + 4][0] * level;
        isDifficult = GameInfo::scoring[clearCount - 1 + 4][1];
        break;
    case 2:
        add += GameInfo::scoring[clearCount - 1 + 7][0] * level;
        isDifficult = 1;
        break;
    }

    isBackToBack = previousClear.isDifficult && (isDifficult || isTSpin > 0);

    if (isBackToBack){
        add = (int)add * 3 / 2;
        b2bCounter++;
    }else{
        b2bCounter = 0;
    }

    add += GameInfo::scoring[16][0] * level * comboCounter;

    if (isPerfectClear)
        add += GameInfo::scoring[clearCount - 1 + 11][0] * level;

    score += add;

    if (clearCount > 0)
        previousClear = Score(clearCount, add, comboCounter, isTSpin, isPerfectClear, isBackToBack, isDifficult);

    sounds.clear = 1;
    clearLock = 1;
    
    return 1;
}

Color Game::color(int n) {
    Color result;
    result.r = GameInfo::colors[n][0];
    result.g = GameInfo::colors[n][1];
    result.b = GameInfo::colors[n][2];

    return result;
}

void Game::fillBag() {
    for (int i = 0; i < 7; i++)
        bag.push_back(i);
}

void Game::next() {
    pawn.y = (int)lengthY / 2 - 1;
    pawn.x = (int)lengthX / 2 - 2;
    pawn.rotation = 0;

    pawn.current = queue.front();
    queue.pop_front();

    fillQueue(1);
    pawn.setBlock();

    if (!checkRotation(0, 0, pawn.rotation))
        lost = 1;
}

void Game::fillQueue(int count) {
    int i;
    for (i = 0; i < count; i++) {
        int n = qran() % bag.size();

        std::list<int>::iterator index = bag.begin();
        std::advance(index, n);

        queue.push_back(*index);
        bag.erase(index);

        if (bag.size() == 0)
            fillBag();
    }
}

void Game::hold() {
    if (!canHold || clearLock)
        return;

    if (held == -1) {
        held = pawn.current;
        next();
    }
    else {
        int temp = held;
        held = pawn.current;
        pawn.current = temp;
        pawn.setBlock();
        pawn.x = (int)lengthX / 2 - 2;
		pawn.y = (int)lengthY / 2 - 1;
    }

    pawn.rotation = 0;
    canHold = 0;
    sounds.hold = 1;
}

int** Game::getShape(int n,int r) {
    int** result = 0;    
    result = new int*[4];

    int i, j;
    for (i = 0; i < 4; i++){
        result[i] = new int[4];
        for (j = 0; j < 4; j++){
            result[i][j] = GameInfo::tetraminos[n][r][i][j]; 
        }
    }

    return result;
}

void Game::lockCheck() {
    if (pawn.lowest == pawn.y) {
        if (lockMoveCounter > 0) {
            lockMoveCounter--;
            lockTimer = maxLockTimer;
        }
    }
}

void Game::keyLeft(int dir) {
    if (clearLock) {
		left = dir;
        return;
    }

    if (left == 0)
        moveLeft();
    left = dir;
    if(dir){
        finesseCounter++;
    }else{
        pushDir = 0;
    }
}

void Game::keyRight(int dir) {
    if (clearLock) {
        right = dir;
        return;
    }

    if (right == 0)
        moveRight();
    right = dir;
    if(dir){
        finesseCounter++;
    }else{
        pushDir = 0;
    }
}

void Game::keyDown(int dir) {
    if (clearLock) {
        down = dir;
        return;
    }

    if (down == 0)
        moveDown();
    down = dir;
}

void Game::removeClearLock() {
    clearLock = 0;
    std::list<int>::iterator index = linesToClear.begin();

    for (int i = 0; i < (int)linesToClear.size(); i++) {
        for (int j = *index; j > 0; j--)
            for (int k = 0; k < lengthX; k++)
                board[j][k] = board[j - 1][k];
        std::advance(index, 1);
    }

    linesToClear = std::list<int>();
    next();
    refresh = 1;
}

void Game::resetSounds(){
    sounds = SoundFlags();
}

void Game::resetRefresh() {
    refresh = 0;
}

void Game::setLevel(int newLevel){
    level = newLevel;
}

void Game::setGoal(int newGoal){
    goal = newGoal;
}

void Game::generateGarbage(int height){
    // shift up
    for(int i = 0; i < lengthY; i++){
        for(int j = 0; j < lengthX; j++){
            if(i < lengthY-height)
                board[i][j] = board[i+height][j];
            else
                board[i][j] = 0;
        }
    }

    for(int i = lengthY-height; i < lengthY; i++){
        int n = qran() % lengthX;

        for(int j = 0; j < lengthX; j++){
            if(j == n)
                continue;
            board[i][j]=8;
        }
    }

    garbageHeight+=height;
}

void Game::keyDrop(){
    dropping = true;
}

Drop Game::getDrop(){
    Drop result = lastDrop;
    lastDrop = Drop();
    return result;
}