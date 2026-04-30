# -*- coding: utf-8 -*-
"""
TeamBA Excel -> JSON 컨버터

BADesign/Excel/*.xlsx 를 읽어 BADesign/Json/<파일명>.json 으로 출력한다.

시트 레이아웃 규약:
  - A 열은 레이블 열이며 데이터로 취급하지 않는다.
  - A 열 셀이 'Client' 인 행이 포함 플래그 행이다. B 열 이후의 정수 값이
    1 이상이면 해당 컬럼을 포함, 0 / 빈셀 / 비숫자면 제외한다.
  - Client 행 바로 아래 행은 B 열부터 필드명(헤더)이다.
  - 데이터 행은 Client 행 두 줄 아래부터 시작한다.
  - 데이터 행의 빈 셀은 JSON 에 넣지 않는다 (언리얼 USTRUCT 기본값으로
    폴백되도록).
"""

import json
import sys
from pathlib import Path

import pandas as pd


def get_base_dir() -> Path:
    if getattr(sys, "frozen", False):
        return Path(sys.executable).resolve().parent.parent
    return Path(__file__).resolve().parent.parent


def to_int_flag(value) -> bool:
    try:
        return int(value) >= 1
    except (TypeError, ValueError):
        return False


def jsonable(value):
    if value is None:
        return None
    if isinstance(value, bool):
        return value
    if isinstance(value, int):
        return value
    if isinstance(value, float):
        if pd.isna(value):
            return None
        if value.is_integer():
            return int(value)
        return value
    if isinstance(value, str):
        return value
    if hasattr(value, "item"):
        try:
            return jsonable(value.item())
        except Exception:
            return str(value)
    return str(value)


def find_client_row(df: pd.DataFrame):
    if df.empty:
        return None
    col0 = df.iloc[:, 0]
    for idx, val in col0.items():
        if val == "Client":
            return idx
    return None


def convert_sheet(df: pd.DataFrame):
    client_r = find_client_row(df)
    if client_r is None:
        return None

    header_r = client_r + 1
    data_start = header_r + 1
    if header_r >= len(df):
        return []

    flag_row = df.iloc[client_r, 1:]
    header_row = df.iloc[header_r, 1:]
    data_block = df.iloc[data_start:, 1:]

    flags = [to_int_flag(v) for v in flag_row]
    headers = []
    for h in header_row:
        if isinstance(h, float) and pd.isna(h):
            headers.append(None)
        else:
            headers.append(h)

    rows = []
    for _, raw in data_block.iterrows():
        record = {}
        for i, (use, name) in enumerate(zip(flags, headers)):
            if not use or name is None:
                continue
            v = jsonable(raw.iloc[i])
            if v is None:
                continue
            record[str(name)] = v
        if record:
            rows.append(record)
    return rows


def convert_excel_to_json() -> int:
    base = get_base_dir()
    in_dir = base / "Excel"
    out_dir = base / "Json"

    if not in_dir.exists():
        in_dir.mkdir(parents=True)
        print(f"[BAConverter] Excel input directory created: {in_dir}")
        return 0

    out_dir.mkdir(parents=True, exist_ok=True)

    files = sorted(p for p in in_dir.glob("*.xlsx") if not p.name.startswith("~$"))
    if not files:
        print("[BAConverter] No .xlsx files found.")
        return 0

    for xlsx in files:
        print(f"[BAConverter] Processing {xlsx.name}")
        try:
            sheets = pd.read_excel(
                xlsx, sheet_name=None, header=None, engine="openpyxl"
            )
        except Exception as e:
            print(f"[BAConverter] Failed to open {xlsx.name}: {e}", file=sys.stderr)
            return 1

        result = {}
        for sheet_name, df in sheets.items():
            rows = convert_sheet(df)
            if rows is None:
                print(
                    f'[BAConverter]   Sheet "{sheet_name}": no Client marker. Skipped.'
                )
                continue
            if not rows:
                print(f'[BAConverter]   Sheet "{sheet_name}": empty. Skipped.')
                continue
            result[sheet_name] = rows
            print(f'[BAConverter]   Sheet "{sheet_name}": {len(rows)} rows.')

        if not result:
            print(f"[BAConverter]   {xlsx.name} produced no output. Skipped.")
            continue

        out_file = out_dir / f"{xlsx.stem}.json"
        with open(out_file, "w", encoding="utf-8") as f:
            json.dump(result, f, ensure_ascii=False, indent=4)
        print(f"[BAConverter]   Saved {out_file}")

    return 0


if __name__ == "__main__":
    sys.exit(convert_excel_to_json())
