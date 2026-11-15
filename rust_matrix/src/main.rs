
use std::io::{self, Write};

// 정수 입력 함수
fn read_number(prompt: &str) -> i64 {
    loop {
        print!("{}", prompt);
        io::stdout().flush().unwrap();

        let mut input = String::new();
        let bytes = io::stdin().read_line(&mut input).unwrap();

        if bytes == 0 {
            std::process::exit(1);
        }

        match input.trim().parse::<i64>() {
            Ok(num) => return num,
            Err(_) => println!("정수를 입력해주세요."),
        }
    }
}

// 행렬 크기 입력
fn read_size(name: &str) -> (usize, usize) {
    loop {
        println!("\n행렬 {}의 크기를 입력하세요.", name);

        let rows = read_number("행: ");
        let cols = read_number("열: ");

        if rows > 0 && cols > 0 {
            return (rows as usize, cols as usize);
        }

        println!("행과 열은 양수여야 합니다.");
    }
}

// Vec을 사용하여 행렬 생성 및 입력
fn read_matrix(name: &str, rows: usize, cols: usize) -> Vec<Vec<i64>> {
    let mut matrix = vec![vec![0; cols]; rows];

    println!("\n행렬 {}의 원소를 입력하세요.", name);

    for i in 0..rows {
        for j in 0..cols {
            matrix[i][j] = read_number(
                &format!("{}[{}][{}]: ", name, i + 1, j + 1)
            );
        }
    }

    matrix
}

fn main() {
    println!("=== Rust 동적 행렬 덧셈 ===");

    let (rows_a, cols_a) = read_size("A");
    let (rows_b, cols_b) = read_size("B");

    if rows_a != rows_b || cols_a != cols_b {
        println!("오류: 두 행렬의 크기가 같아야 합니다.");
        return;
    }

    let a = read_matrix("A", rows_a, cols_a);
    let b = read_matrix("B", rows_b, cols_b);

    let mut result = vec![vec![0; cols_a]; rows_a];

    // 행렬 덧셈
    for i in 0..rows_a {
        for j in 0..cols_a {
            result[i][j] = match a[i][j].checked_add(b[i][j]) {
                Some(value) => value,
                None => {
                    println!("오류: 정수 덧셈 범위를 초과했습니다.");
                    return;
                }
            };
        }
    }

    println!("\n=== 행렬 덧셈 결과 ===");

    for row in &result {
        for value in row {
            print!("{:>6}", value);
        }
        println!();
    }
}

