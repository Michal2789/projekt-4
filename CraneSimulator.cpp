#include <windows.h>
#include <windowsx.h>
#include <vector>
#include <algorithm>
#include <cmath>

enum ShapeType { CIRCLE, TRIANGLE, SQUARE };
enum LiftMode { ALL, ONLY_CIRCLE, ONLY_TRIANGLE, ONLY_SQUARE };
#define BTN_LIFT_ALL       1001
#define BTN_LIFT_CIRCLE    1002
#define BTN_LIFT_TRIANGLE  1003
#define BTN_LIFT_SQUARE    1004
LiftMode liftMode = ALL;  
struct Shape {
    ShapeType type;
    int x, y;
    bool isLifted;
    double weight; 
};

const int shapeSize = 30;
const int groundY = 510;
 double maxCraneLift = 8.0; 
std::vector<Shape> shapes;
int craneX = 300, craneY = 100;
int hookY = 200;
const int hookWidth = 10, hookHeight = 20;
int selectedShape = -1;


std::vector<POINT> GetShapePoints(const Shape& s) {
    std::vector<POINT> points;

    switch (s.type) {
    case CIRCLE: {
        
        for (int i = 0; i < 360; ++i) {
            double angle = 2.0 * 3.1415926535 * i / 360;
            int px = s.x + (int)(shapeSize * cos(angle));
            int py = s.y + (int)(shapeSize * sin(angle));
            points.push_back({ px, py });
        }
        break;
    }

    case TRIANGLE: {
        POINT top = { s.x, s.y - shapeSize };
        POINT left = { s.x - shapeSize, s.y + shapeSize };
        POINT right = { s.x + shapeSize, s.y + shapeSize };

        for (int i = 0; i <= shapeSize; ++i)
            points.push_back({
                top.x + (left.x - top.x) * i / shapeSize,
                top.y + (left.y - top.y) * i / shapeSize
                });

        for (int i = 0; i <= shapeSize; ++i)
            points.push_back({
                left.x + (right.x - left.x) * i / shapeSize,
                left.y + (right.y - left.y) * i / shapeSize
                });

        for (int i = 0; i <= shapeSize; ++i)
            points.push_back({
                right.x + (top.x - right.x) * i / shapeSize,
                right.y + (top.y - right.y) * i / shapeSize
                });
        break;
    }

    case SQUARE: {
        int x1 = s.x - shapeSize;
        int y1 = s.y - shapeSize;

        int x2 = s.x + shapeSize;
        int y2 = s.y + shapeSize;

      
        for (int i = 0; i <= shapeSize; ++i)
            points.push_back({ x1 + i * (x2 - x1) / shapeSize, y1 });

        for (int i = 0; i <= shapeSize; ++i)
            points.push_back({ x2, y1 + i * (y2 - y1) / shapeSize });

        for (int i = 0; i <= shapeSize; ++i)
            points.push_back({ x2 - i * (x2 - x1) / shapeSize, y2 });

        for (int i = 0; i <= shapeSize; ++i)
            points.push_back({ x1, y2 - i * (y2 - y1) / shapeSize });
        break;
    }
    }

    return points;
}

bool IsInsideShape(const std::vector<POINT>& poly, int px, int py) {
    int count = 0;
    size_t n = poly.size();
    for (size_t i = 0; i < n; ++i) {
        POINT p1 = poly[i];
        POINT p2 = poly[(i + 1) % n];

        if (((p1.y >= py) != (p2.y >= py)) &&
            (px <= (p2.x - p1.x) * (py - p1.y) / (double)(p2.y - p1.y) + p1.x)) {
            count++;
        }
    }
    return (count % 2) == 1;
}

bool IsColliding(const Shape& shape1, const Shape& shape2) {
    std::vector<POINT> poly1 = GetShapePoints(shape1);
    std::vector<POINT> poly2 = GetShapePoints(shape2);

    for (const auto& pt : poly1) {
        if (IsInsideShape(poly2, pt.x, pt.y)) {
            return true; 
        }
    }
    return false;
}



void ApplyGravity() {
    const int gravityStep = 5;
    for (auto& s : shapes) {
        if (s.isLifted) continue;

        int nextY = s.y + gravityStep;
        if (nextY > groundY) {
            s.y = groundY;
            continue;
        }

        Shape test = s;
        test.y = nextY;

        bool collides = false;
        for (const auto& other : shapes) {
            if (&s != &other && !other.isLifted && IsColliding(test, other)) {
                collides = true;
                break;
            }
        }

        if (!collides) s.y = nextY;
    }
}

void LiftNearestShape() {
    if (shapes.empty()) return;

    int hookCenterX = craneX;
    int hookCenterY = hookY + hookHeight;
    int nearestIndex = -1;
    double minDist = 1e9;

    for (int i = 0; i < (int)shapes.size(); ++i) {
        if (shapes[i].isLifted) continue;

        bool canLift = false;
        switch (liftMode) {
        case ALL:
            canLift = true;
            break;
        case ONLY_CIRCLE:
            canLift = (shapes[i].type == CIRCLE);
            break;
        case ONLY_TRIANGLE:
            canLift = (shapes[i].type == TRIANGLE);
            break;
        case ONLY_SQUARE:
            canLift = (shapes[i].type == SQUARE);
            break;
        }

        if (!canLift) continue;

        double dx = shapes[i].x - hookCenterX;
        double dy = shapes[i].y - hookCenterY;
        double dist = sqrt(dx * dx + dy * dy);

        if (dist < minDist && dist <= 50.0) {
            minDist = dist;
            nearestIndex = i;
        }
    }

    for (auto& s : shapes) s.isLifted = false;

    if (nearestIndex != -1) {
        if (shapes[nearestIndex].weight > maxCraneLift) {
            MessageBox(NULL, L"Dźwig nie może podnieść tej figury – jest za ciężka!", L"Błąd", MB_OK | MB_ICONWARNING);
            selectedShape = -1;
            return;
        }
        shapes[nearestIndex].isLifted = true;
        selectedShape = nearestIndex;
    }
    else {
        selectedShape = -1;
    }
}



void DrawCrane(HDC hdc) {
    HPEN hPenOld = (HPEN)SelectObject(hdc, CreatePen(PS_SOLID, 4, RGB(0, 0, 0)));

    int baseY = 540, baseX = 70, baseWidth = 50;
    MoveToEx(hdc, baseX, craneY - 70, NULL);
    LineTo(hdc, baseX, baseY);
    MoveToEx(hdc, baseX - 30, baseY, NULL);
    LineTo(hdc, 780, baseY);
    MoveToEx(hdc, baseX + baseWidth, craneY - 70, NULL);
    LineTo(hdc, baseX + baseWidth, baseY);
    MoveToEx(hdc, baseX, baseY, NULL);
    LineTo(hdc, baseX + baseWidth, baseY);
    MoveToEx(hdc, baseX, craneY - 70, NULL);
    LineTo(hdc, baseX + baseWidth, craneY - 70);
    MoveToEx(hdc, baseX + baseWidth, craneY, NULL);
    LineTo(hdc, 760, craneY);
    MoveToEx(hdc, baseX + baseWidth, craneY - 40, NULL);
    LineTo(hdc, 720, craneY - 40);
    LineTo(hdc, 760, craneY);
    Rectangle(hdc, 30, 120, 70, 50);

    DeleteObject(SelectObject(hdc, hPenOld));

    HPEN hPenLine = (HPEN)SelectObject(hdc, CreatePen(PS_SOLID, 2, RGB(0, 0, 0)));
    MoveToEx(hdc, craneX, craneY, NULL);
    LineTo(hdc, craneX, hookY);
    SelectObject(hdc, hPenLine);
    DeleteObject(hPenLine);

    HPEN hPenHook = CreatePen(PS_SOLID, 3, RGB(255, 0, 0));
    HPEN hPenPrev = (HPEN)SelectObject(hdc, hPenHook);
    Rectangle(hdc, craneX - hookWidth, hookY, craneX + hookWidth, hookY + hookHeight);
    SelectObject(hdc, hPenPrev);
    DeleteObject(hPenHook);

    wchar_t liftBuf[64];
    swprintf_s(liftBuf, L"Udźwig dźwigu: %.1f kg", maxCraneLift);
    TextOut(hdc, 20, 10, liftBuf, lstrlen(liftBuf));

    if (selectedShape >= 0 && selectedShape < (int)shapes.size() && shapes[selectedShape].isLifted) {
        const Shape& s = shapes[selectedShape];
        const wchar_t* name = L"";

        switch (s.type) {
        case CIRCLE:   name = L"Okrąg"; break;
        case TRIANGLE: name = L"Trójkąt"; break;
        case SQUARE:   name = L"Kwadrat"; break;
        }

        wchar_t nameBuf[64];
        swprintf_s(nameBuf, L"Trzymany element: %s", name);

        SIZE liftSize;
        GetTextExtentPoint32(hdc, liftBuf, lstrlen(liftBuf), &liftSize);

 
        int posX = 20 + liftSize.cx + 10;
        int posY = 10;  
        TextOut(hdc, posX, posY, nameBuf, lstrlen(nameBuf));
    }

}


void DrawShape(HDC hdc, const Shape& s) {
    int x = s.x;
    int y = s.y;

    HPEN hPenOld = (HPEN)SelectObject(hdc, GetStockObject(BLACK_PEN));
    HBRUSH hBrushOld = (HBRUSH)SelectObject(hdc, GetStockObject(WHITE_BRUSH));

    switch (s.type) {
    case CIRCLE:
        Ellipse(hdc, x - shapeSize, y - shapeSize, x + shapeSize, y + shapeSize);
        break;

    case TRIANGLE: {
        POINT pts[3] = {
            { x, y - shapeSize },
            { x - shapeSize, y + shapeSize },
            { x + shapeSize, y + shapeSize }
        };
        Polygon(hdc, pts, 3);
        break;
    }

    case SQUARE:
        Rectangle(hdc, x - shapeSize, y - shapeSize, x + shapeSize, y + shapeSize);
        break;
    }


    wchar_t buf[64];
    swprintf_s(buf, L"%.1fkg", s.weight);

    SIZE textSize;
    GetTextExtentPoint32(hdc, buf, lstrlen(buf), &textSize);
    int textX = x - textSize.cx / 2;
    int textY = y - textSize.cy / 2;

    TextOut(hdc, textX, textY, buf, lstrlen(buf));

    SelectObject(hdc, hPenOld);
    SelectObject(hdc, hBrushOld);
}


void AddShape(ShapeType type) {
   
    int count = 0;
    for (const auto& s : shapes) {
        if (s.type == type) count++;
    }
    if (count >= 3) {
        MessageBox(NULL, L"Nie można dodać więcej niż 3 figury tego samego typu!", L"Błąd", MB_OK | MB_ICONWARNING);
        return;
    }

    const int startX = 700;
    const int startY = groundY;

    for (const auto& s : shapes) {
        int dx = abs(s.x - startX);
        int dy = abs(s.y - startY);
        if (dx < shapeSize * 2 && dy < shapeSize * 2) {
            MessageBox(NULL, L"Nie można dodać figury – pole startowe jest zajęte.", L"Błąd", MB_OK | MB_ICONWARNING);
            return;
        }
    }
    double weight = 0.0;
    switch (type) {
    case CIRCLE:   weight = 5.0; break;
    case TRIANGLE: weight = 7.5; break;
    case SQUARE:   weight = 10.0; break;
    }

    Shape s = { type, startX, startY, false, weight };
    shapes.push_back(s);
    selectedShape = (int)shapes.size() - 1;
}

void DropSelected() {
    if (selectedShape >= 0 && selectedShape < shapes.size())
        shapes[selectedShape].isLifted = false;
}

void DeleteShapeAtStartPosition() {
    for (int i = 0; i < (int)shapes.size(); ++i) {
        if (shapes[i].x == 700 && shapes[i].y == groundY) {
            shapes.erase(shapes.begin() + i);
            if (selectedShape == i) selectedShape = -1;
            else if (selectedShape > i) selectedShape--;
            break;
        }
    }
}

LRESULT CALLBACK WndProc(HWND hwnd, UINT msg, WPARAM wParam, LPARAM lParam) {
    switch (msg) {
    case WM_CREATE:
    {
        CreateWindow(L"BUTTON", L"Wszystkie figury", WS_VISIBLE | WS_CHILD,
            145, 65, 130, 30, hwnd, (HMENU)BTN_LIFT_ALL, NULL, NULL);
        CreateWindow(L"BUTTON", L"Tylko koła", WS_VISIBLE | WS_CHILD,
            285, 65, 130, 30, hwnd, (HMENU)BTN_LIFT_CIRCLE, NULL, NULL);
        CreateWindow(L"BUTTON", L"Tylko trójkąty", WS_VISIBLE | WS_CHILD,
            425, 65, 130, 30, hwnd, (HMENU)BTN_LIFT_TRIANGLE, NULL, NULL);
        CreateWindow(L"BUTTON", L"Tylko kwadraty", WS_VISIBLE | WS_CHILD,
            565, 65, 130, 30, hwnd, (HMENU)BTN_LIFT_SQUARE, NULL, NULL);
    }
    break;
    case WM_COMMAND:
        switch (LOWORD(wParam))
        {
        case BTN_LIFT_ALL:
            liftMode = ALL;
            MessageBox(hwnd, L"Tryb podnoszenia: Wszystkie figury", L"Tryb", MB_OK);
            break;

        case BTN_LIFT_CIRCLE:
            liftMode = ONLY_CIRCLE;
            MessageBox(hwnd, L"Tryb podnoszenia: Tylko koła", L"Tryb", MB_OK);
            break;

        case BTN_LIFT_TRIANGLE:
            liftMode = ONLY_TRIANGLE;
            MessageBox(hwnd, L"Tryb podnoszenia: Tylko trójkąty", L"Tryb", MB_OK);
            break;

        case BTN_LIFT_SQUARE:
            liftMode = ONLY_SQUARE;
            MessageBox(hwnd, L"Tryb podnoszenia: Tylko kwadraty", L"Tryb", MB_OK);
            break;

  
        }
        break;
    case WM_PAINT: {
        PAINTSTRUCT ps;
        HDC hdc = BeginPaint(hwnd, &ps);

        DrawCrane(hdc);
        for (auto& s : shapes) {
            if (s.isLifted) {
                Shape temp = s;
                temp.x = craneX;
                temp.y = hookY;

                bool blocked = false;
                for (const auto& other : shapes) {
                    if (&s != &other && IsColliding(temp, other)) {
                        blocked = true;
                        break;
                    }
                }

                if (!blocked) {
                    s.x = temp.x;
                    s.y = temp.y;
                }
            }

            DrawShape(hdc, s);
        }

        EndPaint(hwnd, &ps);
        break;
    }
    case WM_TIMER:
        if (wParam == 1) {
            
            if (selectedShape >= 0 && shapes[selectedShape].isLifted) {
                if (shapes[selectedShape].weight > maxCraneLift) {
                  
                    shapes[selectedShape].isLifted = false;
                    MessageBox(NULL, L"Dźwig stracił siłę! Figura upadła.", L"Uwaga", MB_OK | MB_ICONWARNING);
                }
            }

            ApplyGravity();
            InvalidateRect(hwnd, NULL, TRUE);
        }
        break;
    case WM_LBUTTONDOWN: {
        int mx = GET_X_LPARAM(lParam);
        int my = GET_Y_LPARAM(lParam);
        selectedShape = -1;
        for (int i = (int)shapes.size() - 1; i >= 0; --i) {
            if (IsInsideShape(GetShapePoints(shapes[i]), mx, my)) {
                selectedShape = i;
                break;
            }
        }
        InvalidateRect(hwnd, NULL, TRUE);
        break;
    }
    case WM_KEYDOWN:
        switch (wParam) {
        case '1': AddShape(CIRCLE); InvalidateRect(hwnd, NULL, TRUE); break;
        case '2': AddShape(TRIANGLE); InvalidateRect(hwnd, NULL, TRUE); break;
        case '3': AddShape(SQUARE); InvalidateRect(hwnd, NULL, TRUE); break;
        case '4': DeleteShapeAtStartPosition(); InvalidateRect(hwnd, NULL, TRUE); break;
        case VK_OEM_PLUS:  
        case '=': 
            maxCraneLift += 1.0;
            InvalidateRect(hwnd, NULL, TRUE);
            break;

        case VK_OEM_MINUS: 
        case '_':
            if (maxCraneLift > 1.0)
                maxCraneLift -= 1.0;
            InvalidateRect(hwnd, NULL, TRUE);
            break;
        case VK_UP: {
            int newHookY = max(hookY - 10, craneY + 20);
            bool blocked = false;
            if (selectedShape >= 0 && shapes[selectedShape].isLifted) {
                Shape temp = shapes[selectedShape];
                temp.y = newHookY;
                temp.x = craneX;
                for (const auto& other : shapes) {
                    if (&shapes[selectedShape] != &other && IsColliding(temp, other)) {
                        blocked = true;
                        break;
                    }
                }
            }
            if (!blocked) hookY = newHookY;
            InvalidateRect(hwnd, NULL, TRUE);
            break;
        }
        case 'W':  
            if (selectedShape >= 0 && selectedShape < shapes.size()) {
                shapes[selectedShape].weight += 0.5;
                InvalidateRect(hwnd, NULL, TRUE);
            }
            break;

        case 'S': 
            if (selectedShape >= 0 && selectedShape < shapes.size()) {
                if (shapes[selectedShape].weight > 0.5) {
                    shapes[selectedShape].weight -= 0.5;
                    InvalidateRect(hwnd, NULL, TRUE);
                }
            }
            break;

        case VK_DOWN: {
            int newHookY = min(hookY + 10, groundY);
            bool blocked = false;
            if (selectedShape >= 0 && shapes[selectedShape].isLifted) {
                Shape temp = shapes[selectedShape];
                temp.y = newHookY;
                temp.x = craneX;
                for (const auto& other : shapes) {
                    if (&shapes[selectedShape] != &other && IsColliding(temp, other)) {
                        blocked = true;
                        break;
                    }
                }
            }
            if (!blocked) hookY = newHookY;
            InvalidateRect(hwnd, NULL, TRUE);
            break;
        }

        case VK_LEFT: {
            int newCraneX = max(craneX - 10, 140);
            bool blocked = false;
            if (selectedShape >= 0 && shapes[selectedShape].isLifted) {
                Shape temp = shapes[selectedShape];
                temp.x = newCraneX;
                temp.y = hookY;
                for (const auto& other : shapes) {
                    if (&shapes[selectedShape] != &other && IsColliding(temp, other)) {
                        blocked = true;
                        break;
                    }
                }
            }
            if (!blocked) craneX = newCraneX;
            InvalidateRect(hwnd, NULL, TRUE);
            break;
        }

        case VK_RIGHT: {
            int newCraneX = min(craneX + 10, 700);
            bool blocked = false;
            if (selectedShape >= 0 && shapes[selectedShape].isLifted) {
                Shape temp = shapes[selectedShape];
                temp.x = newCraneX;
                temp.y = hookY;
                for (const auto& other : shapes) {
                    if (&shapes[selectedShape] != &other && IsColliding(temp, other)) {
                        blocked = true;
                        break;
                    }
                }
            }
            if (!blocked) craneX = newCraneX;
            InvalidateRect(hwnd, NULL, TRUE);
            break;
        }

        case 'L': LiftNearestShape(); InvalidateRect(hwnd, NULL, TRUE); break;
        case 'D': DropSelected(); InvalidateRect(hwnd, NULL, TRUE); break;
        }
        break;
    case WM_DESTROY:
        KillTimer(hwnd, 1);
        PostQuitMessage(0);
        break;
    default:
        return DefWindowProc(hwnd, msg, wParam, lParam);
    }
    return 0;
}

int WINAPI wWinMain(HINSTANCE hInstance, HINSTANCE, PWSTR, int nCmdShow) {
    const wchar_t CLASS_NAME[] = L"CraneWindowClass";
    MessageBox(NULL,
        L"Sterowanie:\n"
        L"Strzałki - Poruszanie\n"
        L"1 - Wstaw okrąg\n"
        L"2 - Wstaw trójkąt\n"
        L"3 - Wstaw kwadrat\n"
        L"4 - Usuń figurę z pola startowego\n"
        L"L - Podnieś\n"
        L"D - Upuść\n"
        L"W - Zwiększ wagę wybranej figury\n"
        L"S - Zmniejsz wagę wybranej figury\n"
        L"'+'  zwiększ udzwig dzwigu\n"
        L"'-'  zmniejsz udzwig dzwigu\n"
        L"Udźwig aktualny jest wyświetlany w lewym górnym rogu.",
        L"Sterowanie i udźwig", MB_OK | MB_ICONINFORMATION);
    WNDCLASS wc = {};
    wc.lpfnWndProc = WndProc;
    wc.hInstance = hInstance;
    wc.lpszClassName = CLASS_NAME;
    wc.hbrBackground = (HBRUSH)(COLOR_WINDOW + 1);
    wc.hCursor = LoadCursor(NULL, IDC_ARROW);

    RegisterClass(&wc);

    HWND hwnd = CreateWindowEx(0, CLASS_NAME, L"Projekt 4", WS_OVERLAPPEDWINDOW,
        CW_USEDEFAULT, CW_USEDEFAULT, 800, 600, NULL, NULL, hInstance, NULL);

    if (!hwnd) return 0;

    ShowWindow(hwnd, nCmdShow);
    UpdateWindow(hwnd);
    SetTimer(hwnd, 1, 30, NULL);

    MSG msg = {};
    while (GetMessage(&msg, NULL, 0, 0)) {
        TranslateMessage(&msg);
        DispatchMessage(&msg);
    }

    return 0;
}
