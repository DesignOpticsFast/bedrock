# Coding Standards - Bedrock (Python/Backend)

**Version:** 2.0.0  
**Last Updated:** 2025-01-21  
**Language:** Python 3.11+  
**Framework:** FastAPI  
**Development Environment:** dev-01 (Primary) + Local Machines (Secondary)

---

## Core Principles

### 1. Development Environment First
**ALL CODE CHANGES MUST BE DONE ON DEV-01 FIRST**
- ✅ Build and test on dev-01 before local development
- ✅ Use Tailscale for secure connection to dev-01
- ✅ Test functionality on dev-01
- ✅ Debug issues on dev-01
- ✅ Commit changes from dev-01

---

## Development Workflow

### **Phase 1: Development on dev-01**
```bash
# Connect to dev-01 via Tailscale
ssh -i ~/.ssh/github_phoenix mark@100.97.54.75
cd /home/ec2-user/workspace/bedrock

# Make code changes
# Test build
mkdir -p build && cd build
cmake .. -G Ninja
ninja -k0

# Run tests
ctest --output-on-failure

# Commit and push
git add .
git commit -m "feat(core): add new feature"
git push origin feature-branch
```

### **Phase 2: Local Machine Testing**
```bash
# On local machine
git pull origin feature-branch

# Build and test locally
mkdir -p build && cd build
cmake .. -G Ninja
ninja -k0
./bedrock_app
```

### **Key Requirements**
- ✅ **All changes on dev-01 first**
- ✅ **Test functionality on dev-01**
- ✅ **Debug issues on dev-01**
- ✅ **Commit from dev-01**
- ✅ **Test locally after sync**

---

### 2. Clarity Over Cleverness
Write code that your future self (and others) can understand without a PhD in Python wizardry.

**Good:**
```python
def calculate_monthly_revenue(orders: list[Order]) -> Decimal:
    """Calculate total revenue from orders in the current month."""
    current_month_orders = [o for o in orders if o.date.month == datetime.now().month]
    return sum(order.total for order in current_month_orders)
```

**Bad:**
```python
def cmr(o):
    return sum(x.t for x in o if x.d.m == datetime.now().m)
```

---

### 2. Consistency First
Follow established patterns in the codebase. If adding new patterns, document why.

---

### 3. Fail Fast, Fail Loud
Validate inputs early and raise clear exceptions. Don't let bad data propagate.

**Good:**
```python
def process_payment(amount: Decimal, currency: str) -> Payment:
    if amount <= 0:
        raise ValueError(f"Payment amount must be positive, got {amount}")
    if currency not in SUPPORTED_CURRENCIES:
        raise ValueError(f"Unsupported currency: {currency}")
    # ... process payment
```

**Bad:**
```python
def process_payment(amount, currency):
    # Silently continue with invalid data
    result = api.charge(amount, currency)
    return result
```

---

### 4. Test Everything That Matters
Every new function needs tests. If it's worth writing, it's worth testing.

---

### 5. Security by Default
Never commit secrets. Always validate inputs. Assume all user input is malicious.

---

## Python Standards

### Code Style: PEP 8 with Modifications

**Line Length:** 100 characters (not 79)
```python
# Good
def create_user_with_profile(
    username: str, email: str, full_name: str, bio: str
) -> User:
    pass

# Bad - exceeds 100 chars
def create_user_with_profile(username: str, email: str, full_name: str, bio: str, preferences: dict) -> User:
    pass
```

**Indentation:** 4 spaces (never tabs)

**Quotes:** Single quotes for strings, double quotes for docstrings
```python
name = 'Alice'  # Good
message = "Hello"  # Bad

def greet():
    """This is a docstring."""  # Good - double quotes
    return 'Hello'  # Good - single quotes
```

---

### Naming Conventions

**Functions and Variables:** `snake_case`
```python
def calculate_total_price(items: list[Item]) -> Decimal:
    item_count = len(items)
    base_price = sum(item.price for item in items)
    return base_price
```

**Classes:** `PascalCase`
```python
class UserAccount:
    pass

class PaymentProcessor:
    pass
```

**Constants:** `UPPER_SNAKE_CASE`
```python
MAX_RETRIES = 3
API_TIMEOUT_SECONDS = 30
DEFAULT_PAGE_SIZE = 20
```

**Private Members:** Leading underscore
```python
class BankAccount:
    def __init__(self):
        self._balance = 0  # Private
        self.account_number = "123"  # Public
    
    def _validate_transaction(self):  # Private method
        pass
```

**Module-Level Private:** Leading underscore
```python
# internal_utils.py
_INTERNAL_CONSTANT = "secret"  # Private to module

def public_function():
    pass

def _internal_helper():  # Private to module
    pass
```

---

### Type Hints

**Required for all public functions:**
```python
# Good
def get_user_by_id(user_id: int) -> User | None:
    """Retrieve user by ID."""
    return db.query(User).get(user_id)

def calculate_discount(price: Decimal, rate: float) -> Decimal:
    """Calculate discounted price."""
    return price * Decimal(1 - rate)

# Bad - no type hints
def get_user_by_id(user_id):
    return db.query(User).get(user_id)
```

**Use proper types:**
```python
from typing import Optional, Union
from decimal import Decimal

# Modern syntax (Python 3.10+)
def process(data: dict[str, int] | None) -> list[str]:
    pass

# Legacy syntax (if needed)
def process(data: Optional[Dict[str, int]]) -> List[str]:
    pass
```

---

### Function Design

**Keep functions focused and small:**
```python
# Good - single responsibility
def validate_email(email: str) -> bool:
    """Check if email format is valid."""
    return '@' in email and '.' in email.split('@')[1]

def send_welcome_email(user: User) -> None:
    """Send welcome email to new user."""
    if not validate_email(user.email):
        raise ValueError(f"Invalid email: {user.email}")
    
    email_service.send(
        to=user.email,
        subject="Welcome!",
        body=render_template('welcome.html', user=user)
    )

# Bad - doing too much
def validate_and_send_email(user):
    if '@' not in user.email:
        return False
    msg = f"Hi {user.name}, welcome!"
    smtp.connect()
    smtp.send(msg)
    smtp.close()
    log.info("Sent email")
    return True
```

**Function length:** Prefer <50 lines, max 100 lines
**Arguments:** Prefer <5 arguments, use dataclasses for more

---

### Error Handling

**Use specific exceptions:**
```python
# Good
class InvalidPaymentError(Exception):
    """Raised when payment processing fails."""
    pass

def process_payment(amount: Decimal) -> Payment:
    if amount <= 0:
        raise InvalidPaymentError(f"Amount must be positive: {amount}")
    # ...

# Bad
def process_payment(amount):
    if amount <= 0:
        raise Exception("Bad amount")
```

**Provide clear error messages:**
```python
# Good
raise ValueError(
    f"User {user_id} not found. "
    f"Available users: {list(active_users.keys())}"
)

# Bad
raise ValueError("Not found")
```

**Never use bare except:**
```python
# Bad
try:
    result = risky_operation()
except:
    pass

# Good
try:
    result = risky_operation()
except SpecificError as e:
    logger.error(f"Operation failed: {e}")
    raise
```

---

### Docstrings: Google Style

**Required for all public functions/classes:**
```python
def calculate_shipping_cost(
    weight_kg: float,
    destination: str,
    express: bool = False
) -> Decimal:
    """Calculate shipping cost based on weight and destination.
    
    Args:
        weight_kg: Package weight in kilograms
        destination: Destination country code (ISO 3166-1 alpha-2)
        express: Whether to use express shipping
    
    Returns:
        Shipping cost in USD
    
    Raises:
        ValueError: If weight is negative or destination invalid
    
    Example:
        >>> calculate_shipping_cost(2.5, 'US', express=True)
        Decimal('24.99')
    """
    if weight_kg < 0:
        raise ValueError(f"Weight cannot be negative: {weight_kg}")
    
    if destination not in SUPPORTED_COUNTRIES:
        raise ValueError(f"Unsupported destination: {destination}")
    
    base_cost = Decimal(weight_kg * 5.0)
    return base_cost * Decimal(2.0) if express else base_cost
```

---

## Testing Standards

### AAA Pattern: Arrange, Act, Assert

```python
def test_calculate_discount():
    # Arrange
    original_price = Decimal('100.00')
    discount_rate = 0.2
    
    # Act
    final_price = calculate_discount(original_price, discount_rate)
    
    # Assert
    assert final_price == Decimal('80.00')
```

### Test Naming Convention

**Format:** `test_<function>_<condition>_<expected_result>`

```python
def test_get_user_by_id_existing_user_returns_user():
    """Test that getting an existing user returns the user."""
    pass

def test_get_user_by_id_nonexistent_user_returns_none():
    """Test that getting a non-existent user returns None."""
    pass

def test_process_payment_negative_amount_raises_value_error():
    """Test that negative payment amounts raise ValueError."""
    pass
```

### Coverage Requirements

**Minimum:** 80% overall
**Critical paths:** 100% (authentication, payments, data modification)

```bash
# Check coverage
pytest tests/ --cov=bedrock --cov-report=term-missing

# Fail if below threshold
pytest tests/ --cov=bedrock --cov-fail-under=80
```

### Mock External Dependencies

**Never make real API calls in tests:**
```python
# Good
@pytest.fixture
def mock_bedrock_client(mocker):
    mock = mocker.patch('bedrock.services.bedrock_client')
    mock.invoke_model.return_value = {'response': 'test'}
    return mock

def test_chat_service_with_mock(mock_bedrock_client):
    result = chat_service.send_message("Hello")
    assert result == "test"
    mock_bedrock_client.invoke_model.assert_called_once()

# Bad - makes real API call
def test_chat_service():
    result = chat_service.send_message("Hello")  # Real API call!
    assert result is not None
```

---

## Project Structure

```
bedrock/
├── core/
│   ├── __init__.py
│   ├── config.py          # Configuration, settings
│   ├── exceptions.py      # Custom exceptions
│   └── models.py          # Database models
├── services/
│   ├── __init__.py
│   ├── chat_service.py    # Business logic
│   ├── user_service.py
│   └── payment_service.py
├── api/
│   ├── __init__.py
│   ├── routes/            # API endpoints
│   │   ├── chat.py
│   │   ├── users.py
│   │   └── health.py
│   └── schemas/           # Pydantic schemas
│       ├── chat.py
│       └── user.py
├── utils/
│   ├── __init__.py
│   ├── logging.py         # Logging utilities
│   └── validators.py      # Input validation
└── tests/
    ├── conftest.py        # Pytest fixtures
    ├── test_services/
    ├── test_api/
    └── test_utils/
```

---

## Code Review Checklist

Before submitting a PR, verify:

- [ ] **PEP 8 compliance:** Run `flake8 bedrock/`
- [ ] **Type hints:** All public functions have type hints
- [ ] **Docstrings:** All public functions have Google-style docstrings
- [ ] **Tests written:** New code has tests
- [ ] **Tests passing:** `pytest tests/` succeeds
- [ ] **Coverage maintained:** No decrease in coverage %
- [ ] **No secrets:** No API keys, passwords, tokens committed
- [ ] **Error handling:** Appropriate exceptions with clear messages
- [ ] **Logging:** Meaningful log messages at appropriate levels
- [ ] **Performance:** No obvious performance issues
- [ ] **Security:** Input validation, parameterized queries

---

## Anti-Patterns to Avoid

### 1. Bare Except Clauses
```python
# Bad
try:
    risky_operation()
except:
    pass

# Good
try:
    risky_operation()
except SpecificError as e:
    logger.error(f"Operation failed: {e}")
    handle_error(e)
```

### 2. Mutable Default Arguments
```python
# Bad
def add_item(item, items=[]):
    items.append(item)
    return items

# Good
def add_item(item, items=None):
    if items is None:
        items = []
    items.append(item)
    return items
```

### 3. God Classes/Functions
```python
# Bad
class UserManager:
    def create_user(self): pass
    def delete_user(self): pass
    def send_email(self): pass
    def process_payment(self): pass
    def generate_report(self): pass

# Good - single responsibility
class UserService:
    def create_user(self): pass
    def delete_user(self): pass

class EmailService:
    def send_email(self): pass

class PaymentService:
    def process_payment(self): pass
```

### 4. String Concatenation for SQL
```python
# Bad - SQL injection risk!
query = f"SELECT * FROM users WHERE username = '{username}'"

# Good - parameterized query
query = "SELECT * FROM users WHERE username = :username"
result = db.execute(query, {"username": username})
```

### 5. Magic Numbers
```python
# Bad
if user.age > 18:
    pass

# Good
LEGAL_ADULT_AGE = 18
if user.age > LEGAL_ADULT_AGE:
    pass
```

---

## Git Commit Standards

**Format:** `<type>(<scope>): <subject>`

**Types:**
- `feat`: New feature
- `fix`: Bug fix
- `docs`: Documentation changes
- `style`: Code style changes (formatting, no logic change)
- `refactor`: Code refactoring
- `test`: Adding or updating tests
- `chore`: Maintenance tasks

**Examples:**
```bash
feat(api): add chat streaming endpoint
fix(auth): correct token expiration check
docs(readme): update installation instructions
refactor(services): extract common validation logic
test(chat): add integration tests for conversation flow
```

---

## Performance Guidelines

### 1. Profile Before Optimizing
```python
import cProfile

def profile_function():
    profiler = cProfile.Profile()
    profiler.enable()
    
    # Your code here
    
    profiler.disable()
    profiler.print_stats(sort='cumtime')
```

### 2. Use Async for I/O Operations
```python
# Good - async for I/O
async def fetch_user_data(user_id: int) -> User:
    async with httpx.AsyncClient() as client:
        response = await client.get(f"/users/{user_id}")
        return User(**response.json())

# Bad - blocking I/O
def fetch_user_data(user_id):
    response = requests.get(f"/users/{user_id}")
    return User(**response.json())
```

### 3. Bulk Database Operations
```python
# Good - single query
users = db.query(User).filter(User.id.in_(user_ids)).all()

# Bad - N queries
users = [db.query(User).get(id) for id in user_ids]
```

### 4. Cache Expensive Computations
```python
from functools import lru_cache

@lru_cache(maxsize=128)
def calculate_complex_metric(data_id: int) -> float:
    # Expensive calculation
    return result
```

---

## Security Standards

### 1. Environment Variables for Secrets
```python
# Good
import os
API_KEY = os.getenv('API_KEY')

# Bad
API_KEY = "sk-1234567890abcdef"  # Never!
```

### 2. Parameterized Queries
```python
# Good
db.execute(
    "SELECT * FROM users WHERE email = :email",
    {"email": user_email}
)

# Bad
db.execute(f"SELECT * FROM users WHERE email = '{user_email}'")
```

### 3. Input Validation
```python
from pydantic import BaseModel, EmailStr, constr

class UserCreate(BaseModel):
    email: EmailStr
    username: constr(min_length=3, max_length=50)
    age: int = Field(ge=0, le=150)
```

### 4. Redact Sensitive Data in Logs
```python
# Good
logger.info(f"User logged in: {user.id}")

# Bad
logger.info(f"User logged in: {user.email}, password: {password}")
```

---

## Compliance Commands

```bash
# Format code
black bedrock/

# Check style
flake8 bedrock/

# Type checking
mypy bedrock/

# Run tests with coverage
pytest tests/ --cov=bedrock --cov-report=term-missing

# Security check
bandit -r bedrock/

# Full pre-commit check
black bedrock/ && flake8 bedrock/ && mypy bedrock/ && pytest tests/
```

---

## Version History

### 1.0.0 (2025-01-14)
- Initial coding standards document
- Python/FastAPI best practices
- Testing and security guidelines
- Code review checklist

---

**Maintained By:** Engineering Team  
**Questions:** See `.underlord/docs/GOTCHAS.md` for common issues
